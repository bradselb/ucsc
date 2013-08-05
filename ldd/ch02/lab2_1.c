// this source file contains kernel module that fulfills the requirements of
// lab 2, question 1


#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/utsrelease.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");

// -------------------------------------------------------------------------
// globals
static int nr = 0;
module_param(nr, int, 0);
MODULE_PARM_DESC(nr, "a number");

// -------------------------------------------------------------------------
// function prototypes
static int __init lab2_1_init(void);
static void __exit lab2_1_exit(void);

// -------------------------------------------------------------------------
module_init(lab2_1_init);
module_exit(lab2_1_exit);


// -------------------------------------------------------------------------
static int __init lab2_1_init()
{
	int rc = 0;
	unsigned int actual = LINUX_VERSION_CODE ;
	unsigned int expected = KERNEL_VERSION(2,6,32);

	printk(KERN_INFO "[lab2_1] command line param is: %d\n", nr);
	printk(KERN_INFO "[lab2_1] kernel version string is: %s\n", UTS_RELEASE);
	printk(KERN_INFO "[lab2_1] actual kernel version code is: %08x\n", actual);
	printk(KERN_INFO "[lab2_1] expected kernel version code: %08x\n", expected);

	if (actual != expected) {
		printk(KERN_ALERT "[lab2_1] unexpected kernel version\n");
	}


	return rc;
}

// -------------------------------------------------------------------------
static void __exit lab2_1_exit()
{
	printk(KERN_INFO "[lab2_1] module exit\n");
}

