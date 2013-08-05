#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gfp.h>  // get_zeroed_page(), free_page()
#include <linux/fs.h>  // file_operations
#include <linux/string.h>  // strncpy(), strnlen()
#include <linux/types.h>
#include <linux/cdev.h>
//#include <asm/system.h> 
#include <asm/uaccess.h>


#include "bksbuf.h" // my ioctl numbers, and struct bksbuf_addr


MODULE_LICENSE("GPL");


// -------------------------------------------------------------------------
// fwd decl's
int bksbuf_init(void);
void bksbuf_exit(void);

int bksbuf_open(struct inode *inode, struct file* file);
int bksbuf_close(struct inode *inode, struct file *file);
long bksbuf_ioctl(struct file *, unsigned int , unsigned long);

ssize_t bksbuf_read(struct file *file, char *buf, size_t count, loff_t *offset);
ssize_t bksbuf_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

// -------------------------------------------------------------------------
// globals
static int major = 0; // default to dynamic major 
module_param(major, int, 0);
MODULE_PARM_DESC(major, "device major number");


static unsigned int g_bufsize = 4096; // one page
static char *g_buffer = 0; // the buffer is dynamically allocated
static char *g_bufptr = 0; // where we are in the buffer.
static int g_open = 0; // is the file open? 

static struct bksbuf_addr g_addr;

static struct cdev bksbuf_cdev;

static struct file_operations bksbuf_fops = {
	.owner = THIS_MODULE,
	.read = bksbuf_read,
	.write = bksbuf_write,
	.open = bksbuf_open,
	.unlocked_ioctl = bksbuf_ioctl,
	.release = bksbuf_close
};


// -------------------------------------------------------------------------
module_init(bksbuf_init);
module_exit(bksbuf_exit);

// -------------------------------------------------------------------------
// this fctn gets called when the module is loaded. 
int __init bksbuf_init() 
{
	int rc;
	dev_t devid;
	printk(KERN_INFO "bksbuf_init()\n"); 


	g_buffer = 0;
	g_bufptr = g_buffer;
	g_open = 0;

	// this is the old way to do it.
	//rc = register_chrdev(major, "bksbuf", &bksbuf_fops);

	// can pass a major number on the cmdline or let kernel choose one
	if (major) {
		devid = MKDEV(major, 0);
		rc = register_chrdev_region(devid, 1,  "bksbuf");
	} else {
		rc = alloc_chrdev_region(&devid, 0, 1, "bksbuf");
		major = MAJOR(devid);
	}

	if (rc < 0) {
		printk(KERN_ERR "bksbuf_init(): attempt to register character device number %d failed.\n", major);
		// rc = as set by call to register_chrdev(), above.
	} else {
		// success
		printk(KERN_INFO "bksbuf:  driver assigned major number: %d\n", major);
		cdev_init(&bksbuf_cdev, &bksbuf_fops);
		cdev_add(&bksbuf_cdev, devid, 1);
		rc = 0;
	}

	return rc;
}

// -------------------------------------------------------------------------
// this gets called when the module is unloaded. 
void __exit bksbuf_exit()
{
	printk(KERN_INFO "bksbuf_exit()\n");

	cdev_del(&bksbuf_cdev);
	unregister_chrdev_region(MKDEV(major,0), 1);
	if (g_addr.virt) {
		free_page(g_addr.virt);
	}
}

// -------------------------------------------------------------------------
// this gets called when user program calls fopen()
int bksbuf_open(struct inode* inode, struct file* file)
{
	int rc = -1;

	printk(KERN_INFO "bksbuf_open()\n"); 
 
	if (g_open) { 
		rc = -EBUSY;
	} else {
		g_open = 1;
		g_addr.virt = get_zeroed_page(GFP_KERNEL);
		g_addr.phys = __pa(g_addr.virt);
		g_buffer = (char*)g_addr.virt;
		printk(KERN_INFO "bksbuf: allocated page at virtual: 0x%08lx, physical: 0x%08lx\n", g_addr.virt, g_addr.phys);
		rc = 0;
	}
	return rc;
}

// -------------------------------------------------------------------------
// called by fclose()
int bksbuf_close(struct inode* inode, struct file* file)
{
	int rc = 0;

	printk(KERN_INFO "bksbuf_close()\n"); 

	if (g_addr.virt) {
		free_page(g_addr.virt);
	}
	g_addr.virt = 0;
	g_addr.phys = 0;
	g_buffer = 0;
	g_open = 0;

	return rc;
}

// -------------------------------------------------------------------------
long bksbuf_ioctl(struct file *file, unsigned int request, unsigned long arg)
{
	long rc = 0;
	const int dir = _IOC_DIR(request);
	const int type = _IOC_TYPE(request);
	const int number = _IOC_NR(request);
	const int size = _IOC_SIZE(request);

	//printk(KERN_INFO "bksbuf_ioctl(f, %u, %lu)\n", request, arg);
	//printk(KERN_INFO "bksbuf_ioctl(), dir: %d, type: %d, number: %d, size: %d\n", dir, type, number, size);

	if (!arg) {
		rc = -1;
	} else if (BKSBUF_GET_STRLEN == request) {
		int len = 0;
		len = strnlen(g_buffer, g_bufsize-1);
		rc = copy_to_user((int*)arg, &len, sizeof(int));
	} else if (BKSBUF_GET_ADDRESS == request) {
		rc = copy_to_user((struct bksbuf_addr*)arg, &g_addr, sizeof(struct bksbuf_addr));
	} else {
		rc = -1;
	}

	return rc;
}

// -------------------------------------------------------------------------
// just give the user the whole message every time, ignoring offset, etc.
ssize_t bksbuf_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	long ec = -1;

	printk(KERN_INFO "bksbuf_read()\n"); 

	if (count < g_bufsize) {
		bytes_to_copy = count;
	} else {
		bytes_to_copy = g_bufsize;
	}

	ec = copy_to_user(buf, g_buffer, bytes_to_copy);
	if (!ec) {
		bytes_transferred = bytes_to_copy;
	} else {
		bytes_transferred = 0;
	}
	//printk(KERN_INFO "bksbuf_read(): byte transfer count is: %ld\n", bytes_transferred);

	return bytes_transferred;
}


// -------------------------------------------------------------------------
// 
ssize_t bksbuf_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	long ec;

	printk(KERN_INFO "bksbuf_write()\n"); 

	if (g_bufsize < count) {
		bytes_to_copy = g_bufsize;
	} else {
		bytes_to_copy = count;
	}

	ec = -1;
	if (buf && bytes_to_copy) {
		memset(g_buffer, 0, g_bufsize);
		ec = copy_from_user(g_buffer, buf, bytes_to_copy);
	}

	if (!ec) { 
		bytes_transferred = bytes_to_copy;
	} else {
		bytes_transferred = 0;
	}

	return bytes_transferred;
}



