#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/memory.h>
#include <linux/mm_types.h>
#include <linux/mm.h>

#include <linux/jiffies.h>


// -------------------------------------------------------------------------
// globals
static const char* BKS_PROC_FILE_NAME = "bks";
static struct proc_dir_entry* bks_proc_dir_entry;



// -------------------------------------------------------------------------
static int bks_proc_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data)
{
	int count = 0;
	long int j0, j1; // jiffies.
	long int diff;

	if ( 0 < offset ) {
		count = 0;
	} else {
		count = snprintf(buf, bufsize, "Hello World!\n");
		j0 = jiffies;
		j1 = jiffies;
		while ( (diff = (j1 - j0) * 1000 / HZ) < 500) {
			cpu_relax();
			j1 = jiffies;
		}

		count += snprintf(buf+count, bufsize-count, "delay: %lu (mS)\n", diff);
		//count += snprintf(buf+count, bufsize-count, "\n");
		//count += snprintf(buf+count, bufsize-count, "\n");
	}

	// either way, tell caller that this is EOF. 
	if (eof) *eof = -1;

	return count;
}


// -------------------------------------------------------------------------
int __init bks_createProcDirEntry(void)
{
	int rc = 0;

	printk(KERN_INFO "[bks] bks_createProcDirEntry()\n");

	bks_proc_dir_entry = create_proc_entry(BKS_PROC_FILE_NAME, 0 , NULL);
	if (!bks_proc_dir_entry) {
		printk(KERN_ERR "[bks] unable to register /proc/%s\n", BKS_PROC_FILE_NAME);
		rc = -ENOMEM;
	} else {
		printk(KERN_INFO "[bks] successfully registered /proc/%s\n", BKS_PROC_FILE_NAME);
		bks_proc_dir_entry->read_proc = bks_proc_read;
		rc = 0;
	}
	return rc;
}


// -------------------------------------------------------------------------
void __exit bks_removeProcDirEntry(void)
{
	printk(KERN_INFO "[bks] bks_removeProcDirEntry()\n");
	remove_proc_entry(BKS_PROC_FILE_NAME , 0);
}

