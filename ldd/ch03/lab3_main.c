#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
//#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/fs.h>  // file_operations
#include <linux/fcntl.h>

#include "lab3_fileops.h" // my file ops functions


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");


// -------------------------------------------------------------------------
// fwd decl's
static int __init lab3_init(void);
static void __exit lab3_exit(void);


// -------------------------------------------------------------------------
module_init(lab3_init);
module_exit(lab3_exit);


// -------------------------------------------------------------------------
// globals
static int lab3_major = 0; // default to dynamic major 
module_param(lab3_major, int, 0);
MODULE_PARM_DESC(lab3_major, "device major number");


static struct cdev* lab3_cdev;

static struct file_operations lab3_fops = {
	.owner = THIS_MODULE,
	.read = lab3_read,
	.write = lab3_write,
	.open = lab3_open,
	//.unlocked_ioctl = lab3_ioctl,
	.release = lab3_close,
};


// -------------------------------------------------------------------------
// this fctn gets called when the module is loaded. 
static int __init lab3_init() 
{
	int rc = 0;
	dev_t devid;
	printk(KERN_INFO "[lab3] init()\n"); 


	// can pass a major number on the cmdline or let kernel choose one
	if (lab3_major) {
		devid = MKDEV(lab3_major, 0);
		rc = register_chrdev_region(devid, device_count,  "lab3");
	} else {
		rc = alloc_chrdev_region(&devid, 0, device_count, "lab3");
		lab3_major = MAJOR(devid);
	}


	if (rc < 0) {
		printk(KERN_ERR "[lab3] init(), attempt to register character device number %d failed.\n", lab3_major);
		lab3_major = 0; // signal exit function that register failed.
		goto exit;
	} else {
		// success
		printk(KERN_INFO "[lab3] major number: %d\n", lab3_major);
	}

	lab3_cdev = cdev_alloc();
	cdev_init(lab3_cdev, &lab3_fops);
	lab3_cdev->owner = THIS_MODULE;
	rc = cdev_add(lab3_cdev, MKDEV(lab3_major,0), device_count);
	printk(KERN_ERR "[lab3] init(), cdev_add() returned: %d\n", rc);

	lab3_initBuffers();

exit:
	return rc;
}

// -------------------------------------------------------------------------
// this gets called when the module is unloaded. 
static void __exit lab3_exit()
{
	printk(KERN_INFO "[lab3] exit()\n");
	
	lab3_freeBuffers();

	if (lab3_cdev) {
		cdev_del(lab3_cdev);
	}

	if (lab3_major) {
		unregister_chrdev_region(MKDEV(lab3_major,0), device_count);
	}
}



