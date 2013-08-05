//#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gfp.h>  // get_zeroed_page(), free_page()
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/string.h>  // strncpy(), strnlen()
#include <asm/uaccess.h> // copy to/from user

#include "bks_buffer.h"

// -------------------------------------------------------------------------
// my cache of BksBuffer objects.
static struct kmem_cache* bksbuffer_cache;

// -------------------------------------------------------------------------
struct BksBuffer
{
	int size;
	unsigned char* begin;
	unsigned char* end; 
	unsigned char* ptr;
	struct semaphore sem;
};

// -------------------------------------------------------------------------
// not used. 
#if 0
static void bksbuffer_constructor(void* p)
{
	if (p) {
		struct BksBuffer* buf = (struct BksBuffer*) p;
		buf->begin = 0;
		buf->end = 0;
		buf->ptr = 0;
	}
}
#endif

// -------------------------------------------------------------------------
// set up slab allocator for buffers objects - but not for the actual buffer
int bksbuffer_init()
{
	int rc = 0;
	struct kmem_cache* cache = 0;
	cache = kmem_cache_create("BksBuffer", sizeof(struct BksBuffer), 0, SLAB_POISON, 0);
	if (cache) {
		bksbuffer_cache = cache;
		rc = 0;
	} else {
		rc = -ENOMEM;
	}
	return rc;

}

// -------------------------------------------------------------------------
void bksbuffer_cleanup()
{
	if (bksbuffer_cache) {
		kmem_cache_destroy(bksbuffer_cache);
		bksbuffer_cache = 0;
	}
}



// -------------------------------------------------------------------------
// allocate a BksBuffer object from the cache and allocate 
// the buffer using get_zeroed_page()
struct BksBuffer* bksbuffer_allocate()
{
	struct BksBuffer* buf = 0;
	if (!bksbuffer_cache) {
		; // fail - return 0
	} else if (0 == (buf = kmem_cache_alloc(bksbuffer_cache, GFP_KERNEL))) {
		; // fail - return 0
	} else if (0 == (buf->begin = (unsigned char*)get_zeroed_page(GFP_KERNEL))) {
		// fail but, need to cleanup a bit. 
		kmem_cache_free(bksbuffer_cache, buf);
		buf = 0;
	} else {
		// success! finish the job
		buf->size = PAGE_SIZE;
		buf->end = buf->begin;
		buf->ptr = buf->begin;
		sema_init(&(buf->sem), 1); // initially unlocked.
	}

	return buf;
}

// -------------------------------------------------------------------------
void bksbuffer_free(struct BksBuffer* buf)
{
	if (buf) {
		if (buf->begin) {
			memset(buf->begin, 0, buf->size);
			free_page((unsigned long)buf->begin);
			buf->begin = 0;
			buf->end = 0;
			buf->ptr = 0;
			buf->size = 0;
		}
		kmem_cache_free(bksbuffer_cache, buf);
	}
}

// -------------------------------------------------------------------------
int bksbuffer_getBufferSize(struct BksBuffer* buf)
{
	int size = 0;
	if (buf) {
		size = buf->size;
	}
	return size;
}

// -------------------------------------------------------------------------
int bksbuffer_getContentLength(struct BksBuffer* buf)
{
	int len = 0;
	if (!buf) {
		len = -EINVAL;
		// acquire the lock
	} else if (0 != down_interruptible(&buf->sem)) {
		printk(KERN_INFO "[bks] getBufferContentLength(), down_interruptible() returned non zero\n");
		len = -EINTR;
	} else {
		len = buf->end - buf->begin;
		// release the lock
		up(&buf->sem);
	}
	return len;
}


// -------------------------------------------------------------------------
long bksbuffer_copyToUser(struct BksBuffer* bksbuf, __user char* buf, unsigned int count)
{
	long bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	unsigned long remain = 0;
	long ec = -1;


	if (!bksbuf || !bksbuf->begin) {
		bytes_transferred = -EINVAL;
		goto exit; // not much we can do.
	}

	// acquire the lock
	if (down_interruptible(&bksbuf->sem)) {
		printk(KERN_INFO "[bksbuffer] copyToUser(), down_interruptible() returned non zero\n");
		bytes_transferred = -EINTR; // interrupted.
		goto exit;
	}


	// from my present position in the buffer, how many bytes of previously
	// written data remain in the buffer...in other words, 
	// how many bytes can I read from the buffer? 
	remain = bksbuf->end - bksbuf->ptr;

	if (count < 0) {
		bytes_to_copy = 0;
	} else if (count < remain) {
		bytes_to_copy = count;
	} else {
		bytes_to_copy = remain;
	}

	if (bytes_to_copy) {
		ec = copy_to_user(buf, bksbuf->ptr, bytes_to_copy);
	}

	if (!ec) { // success.
		bytes_transferred = bytes_to_copy;
	} else { // not success.
		bytes_transferred = 0;
	}

	bksbuf->ptr += bytes_transferred;

	// release the lock
	up(&bksbuf->sem);


exit:
	return bytes_transferred;

}

// -------------------------------------------------------------------------
long bksbuffer_copyFromUser(struct BksBuffer* bksbuf, __user const char* buf, unsigned int count)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	unsigned long length =  0;
	unsigned long remain =  0;
	long ec = -1;

	if (!bksbuf || !bksbuf->begin) {
		bytes_transferred = -EINVAL;
		goto exit; // not much we can do.
	}


	// acquire the lock
	if (down_interruptible(&bksbuf->sem)) {
		printk(KERN_INFO "[bksbuf] copyFromUser(), down_interruptible() returned non zero\n");
		bytes_transferred = -EINTR; // interrupted.
		goto exit;
	}


	// how many bytes of data are already in the buffer?
	length = bksbuf->end - bksbuf->begin;
	// how many bytes of free space remain in the buffer?
	remain =  bksbuf->size - length;

	printk(KERN_INFO "[bksbuffer] copyFromUser(), length: %lu, remain: %lu\n", length, remain);

	if (count < 0) {
		bytes_to_copy = 0;
	} else if (count < remain) {
		bytes_to_copy = count;
	} else {
		bytes_to_copy = remain;
	}

	if (bytes_to_copy) {
		// always add data to the end - no matter where the 
		// read pointer is.
		ec = copy_from_user(bksbuf->end, buf, bytes_to_copy);
	}

	if (!ec) { 
		bytes_transferred = bytes_to_copy;
		bksbuf->end += bytes_transferred;
	} else {
		bytes_transferred = 0;
	}

	// release the lock
	up(&bksbuf->sem);


exit:
	printk(KERN_INFO "[bksbuf] copyFromUser(), transferred %d bytes\n", bytes_transferred);
	return bytes_transferred;

}

// -------------------------------------------------------------------------
loff_t bksbuffer_lseek(struct BksBuffer* bksbuf, loff_t offset, int whence)
{
	loff_t new_offset; // what we return
	loff_t current_offset = 0; // offset when we enter this fctn.
	loff_t potential_offset = 0; // what might be the new offset

	printk(KERN_INFO "[bksbuf] lseek(), offset: %Ld, whence: %d\n", offset, whence);

	if (!bksbuf || !bksbuf->begin) {
		new_offset = -EINVAL;
		goto exit;
	}
	
	// acquire the lock
	if (down_interruptible(&bksbuf->sem)) {
		printk(KERN_INFO "[bksbuf] lseek(), down_interruptible() returned non zero\n");
		new_offset = -EINTR; // interrupted.
		goto exit;
	}

	// what is the offset coming in?
	current_offset = bksbuf->ptr - bksbuf->begin;

	// what it might be if the caller passed sane parameters.
	switch (whence) {

	case 0: // SEEK_SET:
		potential_offset = offset;
		break;

	case 1: // SEEK_CUR:
		potential_offset = current_offset + offset;
		break;

	case 2: // SEEK_END:
		potential_offset = (bksbuf->size -1) + offset;
		break;

	default:
		// this is an invalid value for whence. 
		// the kernel should not allow this...?
		potential_offset = -1; // force code below to return an error code.
		break;

	} // switch

	// check for sanity.
	if (potential_offset < 0 || !(potential_offset < bksbuf->size)) { 
		new_offset = -EINVAL;
	} else {
		// seems sane. update pointer. 
		new_offset = potential_offset;
		bksbuf->ptr = bksbuf->begin + (long)new_offset;
	}

	// release the lock
	up(&bksbuf->sem);

exit:
	return new_offset;
}

