#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");

// -------------------------------------------------------------------------
// globals
static char* msg = 0;
module_param(msg, charp, S_IRUGO);
MODULE_PARM_DESC(msg, "a message to be printed in the message log file");

// -------------------------------------------------------------------------
// function prototypes
static int __init lab2Q2Q3_a_init(void);
static void __exit lab2Q2Q3_a_exit(void);

void lab2Q2Q3_b_print(const char*); // exported by module lab2Q2Q3_b

// -------------------------------------------------------------------------
module_init(lab2Q2Q3_a_init);
module_exit(lab2Q2Q3_a_exit);


// -------------------------------------------------------------------------
static int __init lab2Q2Q3_a_init()
{
	int rc = 0;
	printk(KERN_INFO "[lab2Q2Q3_a] module init\n");

	if (msg) {
		lab2Q2Q3_b_print(msg);
	} else {
		lab2Q2Q3_b_print("The default message");
	}

	return rc;
}

// -------------------------------------------------------------------------
static void __exit lab2Q2Q3_a_exit()
{
	printk(KERN_INFO "[lab2Q2Q3_a] module exit\n");
}

