#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gfp.h>  // get_zeroed_page(), free_page()
#include <linux/fs.h>  // file_operations
#include <linux/fcntl.h> 
#include <linux/string.h>  // strncpy(), strnlen()
//#include <asm/system.h> 
#include <asm/uaccess.h> // copy to/from user


#include "bks_fileops.h"
#include "bks_buffer.h"
#include "bks_opencount.h"
#include "bks_ioctl.h"


// -------------------------------------------------------------------------
struct BksDeviceContext {
	int nr;
	struct BksBuffer* buf;
};





// -------------------------------------------------------------------------
// this gets called when user program calls fopen()
int bks_open(struct inode* inode, struct file* file)
{
	int rc = 0;
	int nr = MINOR(inode->i_rdev);
	struct BksDeviceContext* ctx = 0;

	printk(KERN_INFO "[bks] open() minor: %d, f_flags: 0x%08x\n", nr, file->f_flags);

	bks_incrOpenCount();

	// if we get here, everything seems to have been successful.
	ctx = kmalloc(sizeof(struct BksDeviceContext), GFP_KERNEL);
	if (!ctx) {
		rc = -ENOMEM;
	} else if (0 == (ctx->buf = bksbuffer_allocate())) {
		rc = -ENOMEM;
	} else {
		ctx->nr = nr; // this minor number
		rc = 0; // success.
	}

	// use private data to keep track of device context.
	file->private_data = ctx;


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
		bksbuffer_free(ctx->buf);
		kfree(ctx);
		ctx = 0;
		file->private_data = 0; // prophy.
	}

	return rc;
}


// -------------------------------------------------------------------------
ssize_t bks_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	struct BksDeviceContext* ctx = file->private_data;


	if (!ctx) {
		bytes_transferred = -EINVAL;
	} else {
		printk(KERN_INFO "[bks] read(), context: %p, minor: %d, buf: %p\n", ctx, ctx->nr, ctx->buf);
		bytes_transferred = bksbuffer_copyToUser(ctx->buf, buf, count);
	}

	printk(KERN_INFO "[bks] read(), transferred %d bytes\n", bytes_transferred);
	return bytes_transferred;
}


// -------------------------------------------------------------------------
ssize_t bks_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	struct BksDeviceContext* ctx = file->private_data;

	if (!ctx) {
		bytes_transferred = -EINVAL;
	} else {
		printk(KERN_INFO "[bks] write(), context: %p, minor: %d, buf: %p\n", ctx, ctx->nr, ctx->buf);
		bytes_transferred = bksbuffer_copyFromUser(ctx->buf, buf, count);
	}

	printk(KERN_INFO "[bks] write(), transferred %d bytes\n", bytes_transferred);
	return bytes_transferred;
}




// -------------------------------------------------------------------------
loff_t bks_llseek(struct file* file, loff_t offset, int whence)
{
	loff_t new_offset; // what we return
	struct BksDeviceContext* ctx = file->private_data;

	printk(KERN_INFO "[bks] llseek(), offset: %Ld, whence: %d\n", offset, whence);

	if (!ctx) {
		new_offset = -EINVAL;
	} else { 
		printk(KERN_INFO "[bks] llseek(), context: %p, minor: %d, buf: %p\n", ctx, ctx->nr, ctx->buf);
		new_offset = bksbuffer_lseek(ctx->buf, offset, whence);
	}

	return new_offset;
}




// -------------------------------------------------------------------------
long bks_ioctl(struct file *file, unsigned int request, unsigned long arg)
{
	long rc = 0;
	struct BksDeviceContext* ctx = file->private_data;

	const int dir = _IOC_DIR(request);
	const int type = _IOC_TYPE(request);
	const int number = _IOC_NR(request);
	const int size = _IOC_SIZE(request);
	printk(KERN_INFO "[bks] ioctl(), request: %u, arg: %lu)\n", request, arg);
	printk(KERN_INFO "[bks] ioctl(), data direction: %d, type: %d, number: %d, size: %d\n", dir, type, number, size);
	printk(KERN_INFO "[bks] ioctl(), context: %p\n", ctx);

	if (!ctx || !ctx->buf || !arg) {
		rc = -EINVAL;
	} else if (BKS_IOCTL_OPEN_COUNT == request) {
		int count = bks_getOpenCount();
		rc = copy_to_user((int*)arg, &count, sizeof(int));
	} else if (BKS_IOCTL_BUFSIZE == request) {
		int size = bksbuffer_getBufferSize(ctx->buf);
		rc = copy_to_user((int*)arg, &size, sizeof(int));
	} else if (BKS_IOCTL_CONTENT_LENGTH == request) {
		int len = bksbuffer_getContentLength(ctx->buf);
		rc = copy_to_user((int*)arg, &len, sizeof(int));
	} else {
		rc = -EINVAL;
	}
	
	return rc;
}



