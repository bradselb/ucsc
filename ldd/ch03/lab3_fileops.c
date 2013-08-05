#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gfp.h>  // get_zeroed_page(), free_page()
#include <linux/fs.h>  // file_operations
#include <linux/fcntl.h> 
#include <linux/string.h>  // strncpy(), strnlen()
//#include <asm/system.h> 
#include <asm/uaccess.h>



#include "lab3_fileops.h"



// -------------------------------------------------------------------------
// a local structure definition.
struct Lab3Device
{
	unsigned char* begin;
	unsigned char* end;
	unsigned char* ptr;
	int holdlock; // do I hold the lock? 
};

// -------------------------------------------------------------------------
static unsigned long lab3_buffer_addr[device_count];
static unsigned long lab3_buffer_len[device_count];
static int lab3_buffer_lock[device_count];

// -------------------------------------------------------------------------
int lab3_initBuffers()
{
	int ec = 0;
	int i;
	for(i=0; i<device_count; ++i) {
		lab3_buffer_addr[i] = get_zeroed_page(GFP_KERNEL);
		if (!lab3_buffer_addr[i]) {
			ec = -ENOMEM;
		}
		lab3_buffer_len[i] = 0;
		lab3_buffer_lock[i] = 0;
	}
	return ec;
}


// -------------------------------------------------------------------------
void lab3_freeBuffers()
{
	int i;
	for (i=0; i<device_count; ++i) {
		if (lab3_buffer_addr[i]) {
			free_page(lab3_buffer_addr[i]);
		}
	}
}

// -------------------------------------------------------------------------
// this gets called when user program calls fopen()
int lab3_open(struct inode* inode, struct file* file)
{
	int rc = 0;
	struct Lab3Device* dev=0;
	int nr = MINOR(inode->i_rdev);

	printk(KERN_INFO "[lab3] open() minor: %d, f_flags: 0x%08x\n", nr, file->f_flags); 

	// enforce "only one writer" rule. 
	if (lab3_buffer_lock[nr] && (file->f_flags & (O_WRONLY|O_RDWR))) {
		rc = -EBUSY;
		goto exit;
	} 
	

	dev = kmalloc(sizeof(struct Lab3Device), GFP_KERNEL ); // todo: use the slab allocator.
	if (!dev) {
		rc = -ENOMEM;
		goto exit;
	}

	file->private_data = dev;

	dev->begin = (unsigned char*)lab3_buffer_addr[nr];
	dev->end = dev->begin + lab3_buffer_len[nr];
	dev->ptr = dev->begin;


	if (file->f_flags & (O_WRONLY|O_RDWR)) {
		lab3_buffer_lock[nr] = 1;
		dev->holdlock = 1;

		if (file->f_flags & O_TRUNC) {
			lab3_buffer_len[nr] = 0;
			dev->end = dev->begin;
		} else if (file->f_flags & O_APPEND) {
			dev->end = dev->begin + lab3_buffer_len[nr];
		}

	}

exit:
	return rc;
}

// -------------------------------------------------------------------------
// called by fclose()
int lab3_close(struct inode* inode, struct file* file)
{
	int rc = 0;
	int nr = MINOR(inode->i_rdev);
	struct Lab3Device* dev = file->private_data;

	printk(KERN_INFO "[lab3] close(), minor: %d\n", nr);


	if (dev && dev->holdlock) {
		lab3_buffer_lock[nr] = 0;
	}

	if (dev) {
		lab3_buffer_len[nr] = (unsigned long)dev->end - (unsigned long)dev->begin;
		kfree(dev);
	}

	return rc;
}


// -------------------------------------------------------------------------
ssize_t lab3_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	unsigned long length, remain;
	long ec = -1;

	struct Lab3Device* dev = file->private_data;
	unsigned long off = *offset; //???
	length = (unsigned long)dev->end - (unsigned long)dev->begin;
	remain = (unsigned long)dev->end - (unsigned long)dev->ptr;

	printk(KERN_INFO "[lab3] read(), count: %d, offset: %lu, length: %lu, remainder: %lu\n", count, off, length, remain); 

	if (count < 0) {
		bytes_to_copy = 0;
	} else if (count < remain) {
		bytes_to_copy = count;
	} else {
		bytes_to_copy = remain;
	}

	ec = -1;
	if (dev) {
		ec = copy_to_user(buf, dev->ptr, bytes_to_copy);
	}

	if (!ec) {
		bytes_transferred = bytes_to_copy;
		dev->ptr += bytes_transferred;
	} else {
		bytes_transferred = 0;
	}
	printk(KERN_INFO "[lab3] read(), transferred %d bytes\n", bytes_transferred);

	return bytes_transferred;
}


// -------------------------------------------------------------------------
ssize_t lab3_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	long ec;

	struct Lab3Device* dev = file->private_data;

	unsigned long off = *offset; //???
	unsigned long remain =  (unsigned long)(dev->begin + 4096) - (unsigned long)dev->end;

	printk(KERN_INFO "[lab3] write(), count: %d, offset: %lu, remain: %lu\n", count, off, remain); 

	if (count < 0) {
		bytes_to_copy = 0;
	} else if (count < remain) {
		bytes_to_copy = count;
	} else {
		bytes_to_copy = remain;
	}

	ec = -1;
	if (bytes_to_copy) {
		ec = copy_from_user(dev->end, buf, bytes_to_copy);
	}

	if (!ec) { 
		bytes_transferred = bytes_to_copy;
		//dev->ptr += bytes_transferred;
		dev->end += bytes_transferred;
	} else {
		bytes_transferred = 0;
	}


	return bytes_transferred;
}







#if 0

// -------------------------------------------------------------------------
long lab3_ioctl(struct file *file, unsigned int request, unsigned long arg)
{
	long rc = 0;

#ifdef LAB3_IOCTL
	const int dir = _IOC_DIR(request);
	const int type = _IOC_TYPE(request);
	const int number = _IOC_NR(request);
	const int size = _IOC_SIZE(request);

	//printk(KERN_INFO "[lab3] ioctl(f, %u, %lu)\n", request, arg);
	//printk(KERN_INFO "[lab3] ioctl(), dir: %d, type: %d, number: %d, size: %d\n", dir, type, number, size);

	if (!arg) {
		rc = -1;
	} else if (BKSBUF_GET_STRLEN == request) {
		int len = 0;
		len = strnlen(lab3_buffer, lab3_bufsize-1);
		rc = copy_to_user((int*)arg, &len, sizeof(int));
	} else if (BKSBUF_GET_ADDRESS == request) {
		rc = copy_to_user((struct lab3_addr*)arg, &lab3_addr, sizeof(struct lab3_addr));
	} else {
		rc = -1;
	}
#endif

	return rc;
}

#endif


