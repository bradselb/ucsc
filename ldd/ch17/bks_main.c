#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
//#include <linux/version.h>
//#include <linux/cdev.h>
//#include <linux/fs.h>  // file_operations
//#include <linux/fcntl.h>

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
// this fctn gets called when the module is loaded. 
static int __init bks_init() 
{
	int rc = 0;

	printk(KERN_INFO "[bks] init()\n"); 
	bks_createProcDirEntry();

	return rc;
}

// -------------------------------------------------------------------------
// this gets called when the module is unloaded. 
static void __exit bks_exit()
{
	printk(KERN_INFO "[bks] exit()\n");
	bks_removeProcDirEntry();
}



