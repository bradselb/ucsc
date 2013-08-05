#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
//#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/fs.h>  // file_operations
#include <linux/fcntl.h>

#include "bks_fileops.h" // my file ops functions
#include "bks_procfile.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");


// -------------------------------------------------------------------------
// fwd decl's
static int __init bks_init(void);
static void __exit bks_exit(void);


// -------------------------------------------------------------------------
module_init(bks_init);
module_exit(bks_exit);


// -------------------------------------------------------------------------
// globals
static int bks_major = 0; // default to dynamic major 
module_param(bks_major, int, 0);
MODULE_PARM_DESC(bks_major, "device major number");


static struct cdev* bks_cdev;

static struct file_operations bks_fops = {
	.owner = THIS_MODULE,
	.read = bks_read,
	.write = bks_write,
	.open = bks_open,
	.llseek = bks_llseek,
	.unlocked_ioctl = bks_ioctl,
	.release = bks_close,
};


// -------------------------------------------------------------------------
// this fctn gets called when the module is loaded. 
static int __init bks_init() 
{
	int rc = 0;
	dev_t devid;
	int device_count = bks_getBufferCount();
	printk(KERN_INFO "[bks] init()\n"); 


	// can pass a major number on the cmdline or let kernel choose one
	if (bks_major) {
		devid = MKDEV(bks_major, 0);
		rc = register_chrdev_region(devid, device_count,  "bks");
	} else {
		rc = alloc_chrdev_region(&devid, 0, device_count, "bks");
		bks_major = MAJOR(devid);
	}


	if (rc < 0) {
		printk(KERN_ERR "[bks] init(), attempt to register character device number %d failed.\n", bks_major);
		bks_major = 0; // signal exit function that register failed.
		goto exit;
	} else {
		// success
		printk(KERN_INFO "[bks] major number: %d\n", bks_major);
	}

	bks_cdev = cdev_alloc();
	cdev_init(bks_cdev, &bks_fops);
	bks_cdev->owner = THIS_MODULE;
	rc = cdev_add(bks_cdev, MKDEV(bks_major,0), device_count);
	printk(KERN_ERR "[bks] init(), cdev_add() returned: %d\n", rc);

	bks_initBuffers();
	bks_createProcDirEntry(device_count);

exit:
	return rc;
}

// -------------------------------------------------------------------------
// this gets called when the module is unloaded. 
static void __exit bks_exit()
{
	int device_count = bks_getBufferCount();
	printk(KERN_INFO "[bks] exit()\n");
	
	bks_removeProcDirEntry();
	bks_freeBuffers();

	if (bks_cdev) {
		cdev_del(bks_cdev);
	}

	if (bks_major) {
		unregister_chrdev_region(MKDEV(bks_major,0), device_count);
	}
}



