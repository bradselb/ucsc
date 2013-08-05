#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");

// -------------------------------------------------------------------------
// fwd decl's
int bksinit(void);
void bksexit(void);



// -------------------------------------------------------------------------
module_init(bksinit);
module_exit(bksexit);


int __init bksinit()
{
	int rc = 0;
	struct task_struct* task = current;
	char* buf = (char*)get_zeroed_page(GFP_KERNEL);

	printk(KERN_INFO "bks parent module loaded\n");
	if (buf) {
		get_task_comm(buf, task);
		printk(KERN_INFO "%d:'%s'\n", task->pid, buf);
		while (1 < task->pid) {
			task = task->parent;
			get_task_comm(buf, task);
			printk(KERN_INFO "%d:'%s'\n", task->pid, buf);
		};
		free_page((unsigned long)buf);
	}

	return rc;
}

void __exit bksexit()
{
	printk(KERN_INFO "bks parent module unloaded\n");
}
