#include <linux/module.h>
#include <linux/init.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");

// -------------------------------------------------------------------------
// globals

// -------------------------------------------------------------------------
// function prototypes
static int __init lab2Q2Q3_b_init(void);
static void __exit lab2Q2Q3_b_exit(void);
void lab2Q2Q3_b_print(const char*); // exported by this module


EXPORT_SYMBOL(lab2Q2Q3_b_print);


// -------------------------------------------------------------------------
module_init(lab2Q2Q3_b_init);
module_exit(lab2Q2Q3_b_exit);


// -------------------------------------------------------------------------
static int __init lab2Q2Q3_b_init()
{
	int rc = 0;

	printk(KERN_INFO "[lab2Q2Q3_b] module init\n");

	return rc;
}

// -------------------------------------------------------------------------
static void __exit lab2Q2Q3_b_exit()
{
	printk(KERN_INFO "[lab2Q2Q3_b] module exit\n");
}


// -------------------------------------------------------------------------
void lab2Q2Q3_b_print(const char* msg)
{
	printk(KERN_INFO "[lab2Q2Q3_b] %s\n", msg);
}
