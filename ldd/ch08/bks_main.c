#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
//#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/fs.h>  // file_operations
#include <linux/fcntl.h>

#include "bks_fileops.h" // my file ops functions
#include "bks_buffer.h"
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

static int bks_device_count = 4;
module_param(bks_device_count, int, 4);
MODULE_PARM_DESC(bks_device_count, "how many minor numbers to associate with this device");

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
	printk(KERN_INFO "[bks] init()\n"); 


	// can pass a major number on the cmdline or let kernel choose one
	if (bks_major) {
		devid = MKDEV(bks_major, 0);
		rc = register_chrdev_region(devid, bks_device_count,  "bks");
	} else {
		rc = alloc_chrdev_region(&devid, 0, bks_device_count, "bks");
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
	rc = cdev_add(bks_cdev, MKDEV(bks_major,0), bks_device_count);
	printk(KERN_ERR "[bks] init(), cdev_add() returned: %d\n", rc);

	bksbuffer_init(); // should check return value. 

	bks_createProcDirEntry(0);

exit:
	return rc;
}

// -------------------------------------------------------------------------
// this gets called when the module is unloaded. 
static void __exit bks_exit()
{
	printk(KERN_INFO "[bks] exit()\n");

	bks_removeProcDirEntry();
	
	bksbuffer_cleanup();

	if (bks_cdev) {
		cdev_del(bks_cdev);
	}

	if (bks_major) {
		unregister_chrdev_region(MKDEV(bks_major,0), bks_device_count);
	}
}



