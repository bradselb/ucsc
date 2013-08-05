#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gfp.h>  // get_zeroed_page(), free_page()
#include <linux/fs.h>  // file_operations
#include <linux/fcntl.h> 
#include <linux/string.h>  // strncpy(), strnlen()
//#include <asm/system.h> 
#include <asm/uaccess.h> // copy to/from user
#include <linux/semaphore.h>


#include "bks_fileops.h"
#include "bks_opencount.h"



// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
// globals (with file scope)


static unsigned long bks_buffer_size[device_count]; // allocated
static unsigned char* bks_buffer_begin[device_count]; // always points to begining
static unsigned char* bks_buffer_end[device_count]; // one past last char in buffer
// this semaphore must be acquired prior to either accessing the end pointer. 
// or calling copy to/from user in the read/write method
static struct semaphore bks_buffer_lock[device_count];


// -------------------------------------------------------------------------
int bks_initBuffers()
{
	int ec = 0;
	// initialze the buffers
	int i;
	for(i=0; i<device_count; ++i) {
		bks_buffer_begin[i] = (unsigned char*)get_zeroed_page(GFP_KERNEL);
		bks_buffer_end[i] = bks_buffer_begin[i];
		bks_buffer_size[i] = PAGE_SIZE;
		if (!bks_buffer_begin[i]) {
			ec = -ENOMEM;
			goto exit;
		}

		// initialize semaphores
		sema_init(&bks_buffer_lock[i], 1);
	}

exit:
	return ec;
}


// -------------------------------------------------------------------------
void bks_freeBuffers()
{
	int i;
	for (i=0; i<device_count; ++i) {
		if (bks_buffer_begin[i]) {
			free_page((unsigned long)bks_buffer_begin[i]);
		}
		bks_buffer_begin[i] = 0; 
		bks_buffer_end[i] = 0; 
	}

	// no need to cleanup semaphores?
}


// -------------------------------------------------------------------------
int bks_getBufferSize(int nr)
{
	return bks_buffer_size[nr];
}


// -------------------------------------------------------------------------
int bks_getBufferContentLength(int nr)
{
	int len = 0;
	// acquire the lock
	if (0 != down_interruptible(&bks_buffer_lock[nr])) {
		printk(KERN_INFO "[bks] getBufferContentLength(), down_interruptible() returned non zero\n");
		len = -EINTR;
		goto exit;
	}

	len = bks_buffer_end[nr] - bks_buffer_begin[nr];
	// release the lock
	up(&bks_buffer_lock[nr]);
exit:
	return len;
}


// -------------------------------------------------------------------------
// this gets called when user program calls fopen()
int bks_open(struct inode* inode, struct file* file)
{
	int rc = 0;
	int nr = MINOR(inode->i_rdev);
	unsigned char* ptr = bks_buffer_begin[nr]; // default

	printk(KERN_INFO "[bks] open() minor: %d, f_flags: 0x%08x\n", nr, file->f_flags);

	bks_incrOpenCount();

	// acquire the lock
	if (0 != down_interruptible(&bks_buffer_lock[nr])) {
		printk(KERN_INFO "[bks] open(), down_interruptible() returned non zero\n");
		// interrupted. bail out
		rc = -EINTR;
		goto exit;
	}

	if (file->f_flags & O_TRUNC) {
		bks_buffer_end[nr] = bks_buffer_begin[nr];
		ptr = bks_buffer_begin[nr]; 
	} else if (file->f_flags & O_APPEND) {
		ptr = bks_buffer_end[nr];
	}
	// release the lock
	up(&bks_buffer_lock[nr]);

	// use private data to keep track of where we are in the buffer.
	file->private_data = ptr;


exit:
	return rc;
}

// -------------------------------------------------------------------------
// called by fclose()
int bks_close(struct inode* inode, struct file* file)
{
	int rc = 0;
	int nr = MINOR(inode->i_rdev);

	printk(KERN_INFO "[bks] close(), minor: %d\n", nr);

	return rc;
}


// -------------------------------------------------------------------------
ssize_t bks_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	unsigned long remain = 0;
	int nr = 0;
	long ec = -1;
	unsigned char* ptr = file->private_data;

	printk(KERN_INFO "[bks] read(), my private data is: %p\n", ptr);

	if (!ptr) goto exit; // not much we can do.

	// acquire the lock
	if (down_interruptible(&bks_buffer_lock[nr])) {
		printk(KERN_INFO "[bks] read(), down_interruptible() returned non zero\n");
		bytes_transferred = -EINTR;
		goto exit;
	}

	// if somebody else has truncated the file, then adjust our
	// position so that we're at least looking at something valid. 
	if (bks_buffer_end[nr] < ptr) {
		ptr = bks_buffer_end[nr];
	}

	// from my present position in the buffer, how many bytes of data (that were previously written)
	// remain in the buffer...in other words, how many can I read from the buffer? 
	remain = bks_buffer_end[nr] - ptr;

	if (count < 0) {
		bytes_to_copy = 0;
	} else if (count < remain) {
		bytes_to_copy = count;
	} else {
		bytes_to_copy = remain;
	}

	if (bytes_to_copy) {
		ec = copy_to_user(buf, ptr, bytes_to_copy);
	}

	if (!ec) { // success.
		bytes_transferred = bytes_to_copy;
	} else { // not success.
		bytes_transferred = 0;
	}
	
	ptr += bytes_transferred;
	file->private_data = ptr;

	// release the lock
	up(&bks_buffer_lock[nr]);


exit:
	printk(KERN_INFO "[bks] read(), transferred %d bytes\n", bytes_transferred);
	return bytes_transferred;
}


// -------------------------------------------------------------------------
ssize_t bks_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	unsigned long length =  0;
	unsigned long remain =  0;
	long ec = -1;
	int nr = 0; // oops! we don't really support more than one device yet.
	unsigned char* ptr = file->private_data;

	printk(KERN_INFO "[bks] write(), count: %d, private data: %p\n", count, ptr);

	if (!ptr) goto exit; // not much we can do.

	// acquire the lock
	if (0 != down_interruptible(&bks_buffer_lock[nr])) {
		printk(KERN_INFO "[bks] write(), down_interruptible() returned non zero\n");
		// interrupted. bail out
		bytes_transferred = -EINTR;
		goto exit;
	} else {
		printk(KERN_INFO "[bks] write(), successfully acquired the lock\n");
	}


	// always add data to the end - no matter where we were previously.
	ptr = bks_buffer_end[nr];

	// how many bytes of data are already in the buffer?
	//length = ptr - bks_buffer_begin[nr];
	length = bks_buffer_end[nr] - bks_buffer_begin[nr];
	// how many bytes of free space remains in the buffer?
	remain =  bks_buffer_size[nr] - length;

	printk(KERN_INFO "[bks] write(), length: %lu, remain: %lu\n", length, remain);

	if (count < 0) {
		bytes_to_copy = 0;
	} else if (count < remain) {
		bytes_to_copy = count;
	} else {
		bytes_to_copy = remain;
	}

	if (bytes_to_copy) {
		ec = copy_from_user(ptr, buf, bytes_to_copy);
	}

	if (!ec) { 
		bytes_transferred = bytes_to_copy;
		bks_buffer_end[nr] += bytes_transferred;
	} else {
		bytes_transferred = 0;
	}

	ptr += bytes_transferred;
	file->private_data = ptr;

	// release the lock
	up(&bks_buffer_lock[nr]);


exit:
	printk(KERN_INFO "[bks] write(), transferred %d bytes\n", bytes_transferred);
	return bytes_transferred;
}







#if 0

// -------------------------------------------------------------------------
long bks_ioctl(struct file *file, unsigned int request, unsigned long arg)
{
	long rc = 0;

#ifdef LAB3_IOCTL
	const int dir = _IOC_DIR(request);
	const int type = _IOC_TYPE(request);
	const int number = _IOC_NR(request);
	const int size = _IOC_SIZE(request);

	//printk(KERN_INFO "[bks] ioctl(f, %u, %lu)\n", request, arg);
	//printk(KERN_INFO "[bks] ioctl(), dir: %d, type: %d, number: %d, size: %d\n", dir, type, number, size);

	if (!arg) {
		rc = -1;
	} else if (BKSBUF_GET_STRLEN == request) {
		int len = 0;
		len = strnlen(bks_buffer, bks_bufsize-1);
		rc = copy_to_user((int*)arg, &len, sizeof(int));
	} else if (BKSBUF_GET_ADDRESS == request) {
		rc = copy_to_user((struct bks_addr*)arg, &bks_addr, sizeof(struct bks_addr));
	} else {
		rc = -1;
	}
#endif

	return rc;
}

#endif


