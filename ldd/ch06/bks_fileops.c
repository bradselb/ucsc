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
#include "bks_ioctl.h"


// -------------------------------------------------------------------------
#define BKS_BUFFER_COUNT 4

// -------------------------------------------------------------------------
// globals (with file scope)
static unsigned long bks_buffer_size[BKS_BUFFER_COUNT]; // allocated
static unsigned char* bks_buffer_begin[BKS_BUFFER_COUNT]; // always points to begining
static unsigned char* bks_buffer_end[BKS_BUFFER_COUNT]; // one past last char in buffer
// this semaphore must be acquired prior to either accessing the end pointer. 
// or calling copy to/from user in the read/write method
static struct semaphore bks_buffer_lock[BKS_BUFFER_COUNT];

// -------------------------------------------------------------------------
struct BksDeviceContext {
	int nr;
	unsigned char* ptr;
};

// -------------------------------------------------------------------------
int bks_getBufferCount(void)
{
	return BKS_BUFFER_COUNT;
}

// -------------------------------------------------------------------------
int bks_initBuffers()
{
	int ec = 0;
	// initialze the buffers
	int i;
	for(i=0; i<BKS_BUFFER_COUNT; ++i) {
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
	for (i=0; i<BKS_BUFFER_COUNT; ++i) {
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
	struct BksDeviceContext* ctx = 0;

	printk(KERN_INFO "[bks] open() minor: %d, f_flags: 0x%08x\n", nr, file->f_flags);

	bks_incrOpenCount();

	// acquire the lock
	if (0 != down_interruptible(&bks_buffer_lock[nr])) {
		printk(KERN_INFO "[bks] open(), down_interruptible() returned non zero\n");
		// interrupted. bail out
		rc = -EINTR;
		goto exit;
	}

	// manipulate the globals stuff to effect an open.
	if (file->f_flags & O_TRUNC) {
		bks_buffer_end[nr] = bks_buffer_begin[nr];
		ptr = bks_buffer_begin[nr]; 
	} else if (file->f_flags & O_APPEND) {
		ptr = bks_buffer_end[nr];
	}

	// release the lock
	up(&bks_buffer_lock[nr]);

	// if we get here, everything seems to have been successful.
	ctx = kmalloc(sizeof(struct BksDeviceContext), GFP_KERNEL);
	if (!ctx) {
		rc = -ENOMEM;
	} else {
		ctx->nr = nr; // this minor number
		ctx->ptr = ptr; // where are we in the buffer?
	}

	// use private data to keep track of device context.
	file->private_data = ctx;


exit:
	return rc;
}

// -------------------------------------------------------------------------
// called by fclose()
int bks_close(struct inode* inode, struct file* file)
{
	int rc = 0;
	int nr = MINOR(inode->i_rdev);
	struct BksDeviceContext* ctx = file->private_data;

	printk(KERN_INFO "[bks] close(), minor: %d\n", nr);
	if (ctx) {
		kfree(ctx);
	}

	return rc;
}


// -------------------------------------------------------------------------
ssize_t bks_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	unsigned long remain = 0;
	long ec = -1;
	struct BksDeviceContext* ctx = file->private_data;
	int nr = 0;
	unsigned char* ptr = 0;


	if (!ctx) {
		bytes_transferred = -EINVAL;
		goto exit; // not much we can do.
	} else {
		nr = ctx->nr;
		ptr = ctx->ptr;
	}
	printk(KERN_INFO "[bks] read(), context: %p, minor: %d, ptr: %p\n", ctx, nr, ptr);

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

	// from my present position in the buffer, how many bytes of data (that were
	// previously written) remain in the buffer...
	// in other words, how many bytes can I read from the buffer? 
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
	ctx->ptr = ptr;

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
	struct BksDeviceContext* ctx = file->private_data;
	int nr = 0;
	unsigned char* ptr = 0;

	if (!ctx) {
		bytes_transferred = -EINVAL;
		goto exit; // not much we can do.
	} else {
		nr = ctx->nr;
		ptr = ctx->ptr;
	}
	printk(KERN_INFO "[bks] write(), context: %p, minor: %d, ptr: %p\n", ctx, nr, ptr);

	// acquire the lock
	if (0 != down_interruptible(&bks_buffer_lock[nr])) {
		printk(KERN_INFO "[bks] write(), down_interruptible() returned non zero\n");
		// interrupted. bail out
		bytes_transferred = -EINTR;
		goto exit;
	}


	// always add data to the end - no matter where we were previously.
	ptr = bks_buffer_end[nr];

	// how many bytes of data are already in the buffer?
	//length = ptr - bks_buffer_begin[nr];
	length = bks_buffer_end[nr] - bks_buffer_begin[nr];
	// how many bytes of free space remains in the buffer?
	remain =  bks_buffer_size[nr] - length; // getBufferAvailableSpace(nr)

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
	ctx->ptr = ptr;

	// release the lock
	up(&bks_buffer_lock[nr]);


exit:
	printk(KERN_INFO "[bks] write(), transferred %d bytes\n", bytes_transferred);
	return bytes_transferred;
}




// -------------------------------------------------------------------------
loff_t bks_llseek(struct file* file, loff_t offset, int whence)
{
	loff_t new_offset; // what we return
	loff_t current_offset = 0; // offset when we enter this fctn.
	loff_t potential_offset = 0; // what might be the new offset
	struct BksDeviceContext* ctx = file->private_data;
	int nr = 0;
	unsigned char* ptr = 0;

	printk(KERN_INFO "[bks] llseek(), offset: %Ld, whence: %d\n", offset, whence);

	if (!ctx) {
		new_offset = -EINVAL;
		goto exit; // not much we can do.
	} else {
		nr = ctx->nr;
		ptr = ctx->ptr;
	}
	printk(KERN_INFO "[bks] llseek(), context: %p, minor: %d, ptr: %p\n", ctx, nr, ptr);

	// acquire the lock
	if (0 != down_interruptible(&bks_buffer_lock[nr])) {
		printk(KERN_WARNING "[bks] llseek(), down_interruptible() returned non zero\n");
		// interrupted. bail out
		new_offset = -EINTR;
		goto exit;
	}
	
	
	// --------------------------------------------------------------------
	// do the llseek.

	// what is the offset coming in?
	current_offset = ptr - bks_buffer_begin[nr];

	// what it might be if the caller passed sane parameters.
	switch (whence) {

	case SEEK_SET:
		potential_offset = offset;
		break;

	case SEEK_CUR:
		potential_offset = current_offset + offset;
		break;

	case SEEK_END:
		potential_offset = (bks_buffer_size[nr] -1) + offset;
		break;

	default:
		// this is an invalid value for whence. 
		// the kernel should not allow this...?
		potential_offset = -1; // force code below to return an error code.
		break;

	} // switch

	// check for sanity.
	if (potential_offset < 0 || !(potential_offset < bks_buffer_size[nr])) { 
		new_offset = -EINVAL;
	} else {
		// seems sane. update pointer. 
		new_offset = potential_offset;
		ptr = bks_buffer_begin[nr] + (long)new_offset;
		ctx->ptr = ptr;
printk(KERN_INFO "[bks] llseek(), current_offset: %Ld, new_offset: %Ld, ptr; %p\n", current_offset, new_offset, ptr);
	}

	// release the lock
	up(&bks_buffer_lock[nr]);

exit:
	return new_offset;
}



// -------------------------------------------------------------------------
unsigned int bks_poll(struct file* file, struct poll_table_struct* polltab)
{
	unsigned int rc = 0;
	struct BksDeviceContext* ctx = file->private_data;
	int nr = 0;
	unsigned char* ptr = 0;

	printk(KERN_INFO "[bks] poll(), \n");

	if (!ctx) {
		rc = -EINVAL;
		goto exit; // not much we can do.
	} else {
		nr = ctx->nr;
		ptr = ctx->ptr;
	}

exit:
	return rc;
}


// -------------------------------------------------------------------------
long bks_ioctl(struct file *file, unsigned int request, unsigned long arg)
{
	long rc = 0;
	struct BksDeviceContext* ctx = file->private_data;
	int nr = 0;
	unsigned char* ptr = 0;

	const int dir = _IOC_DIR(request);
	const int type = _IOC_TYPE(request);
	const int number = _IOC_NR(request);
	const int size = _IOC_SIZE(request);
	printk(KERN_INFO "[bks] ioctl(), request: %u, arg: %lu)\n", request, arg);
	printk(KERN_INFO "[bks] ioctl(), data direction: %d, type: %d, number: %d, size: %d\n", dir, type, number, size);

	if (!ctx) {
		rc = -EINVAL;
		goto exit; // not much we can do.
	} else {
		nr = ctx->nr;
		ptr = ctx->ptr;
	}
	printk(KERN_INFO "[bks] ioctl(), context: %p, minor: %d, ptr: %p\n", ctx, nr, ptr);

	if (!arg) {
		rc = -1;
	} else if (BKS_IOCTL_OPEN_COUNT == request) {
		int count = bks_getOpenCount();
		rc = copy_to_user((int*)arg, &count, sizeof(int));
	} else if (BKS_IOCTL_BUFSIZE == request) {
		int size = bks_getBufferSize(nr);
		rc = copy_to_user((int*)arg, &size, sizeof(int));
	} else if (BKS_IOCTL_CONTENT_LENGTH == request) {
		int len = bks_getBufferContentLength(nr);
		rc = copy_to_user((int*)arg, &len, sizeof(int));
	} else {
		rc = -1;
	}

exit:
	return rc;
}



