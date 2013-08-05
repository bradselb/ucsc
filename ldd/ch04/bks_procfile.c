#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/memory.h>
#include <linux/mm_types.h>
#include <linux/mm.h>

#include "bks_fileops.h" // buffer length and size

// -------------------------------------------------------------------------
// globals
static const char* BKS_PROC_FILE_NAME = "bks";
static struct proc_dir_entry* bks_proc_dir_entry;
static int bks_group = 42; // default to the answer to the universe 
module_param(bks_group, int, 0);
MODULE_PARM_DESC(bks_group, "group number");



// -------------------------------------------------------------------------
static int bks_proc_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data)
{
	int count = 0;

	if ( 0 < offset ) {
		count = 0;
	} else {
		count = snprintf(buf, bufsize, "Group number: %d\n", bks_group);
		count += snprintf(buf+count, bufsize-count, "Group members: Brad Selbrede\n");
		count += snprintf(buf+count, bufsize-count, "Buffer size: %d\n", bks_getBufferSize(0));
		count += snprintf(buf+count, bufsize-count, "Buffer content length: %d\n", bks_getBufferContentLength(0));
		//count += snprintf(buf+count, bufsize-count, "\n");
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

