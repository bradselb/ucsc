#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/memory.h>
#include <linux/mm_types.h>
#include <linux/mm.h>

#include "bks_fileops.h" // buffer length and size
#include "bks_devicecontext.h"
#include "bks_ringbuf.h"
#include "bks_opencount.h"

// -------------------------------------------------------------------------
// forward decl's
static int bks_procfile_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data);

// -------------------------------------------------------------------------
// globals
static const char* BKS_PROC_DIR_NAME = "bks";

static int bks_group = 42; // default to the answer to the universe 
module_param(bks_group, int, 0);
MODULE_PARM_DESC(bks_group, "group number");



// -------------------------------------------------------------------------
int __init bks_createProcDirEntry(void* private_data)
{
	int rc = 0;
	struct proc_dir_entry* entry = 0;

	printk(KERN_INFO "[bks] bks_createProcDirEntry()\n");

	entry = create_proc_read_entry(BKS_PROC_DIR_NAME, 0, 0, bks_procfile_read, private_data);

	if (!entry) {
		printk(KERN_ERR "[bks] unable to create dir /proc/%s\n", BKS_PROC_DIR_NAME);
		rc = -ENOMEM;
	}

	return rc;
}


// -------------------------------------------------------------------------
void __exit bks_removeProcDirEntry(void)
{
	printk(KERN_INFO "[bks] bks_removeProcDirEntry()\n");

	remove_proc_entry(BKS_PROC_DIR_NAME, 0);

}




// -------------------------------------------------------------------------
static int bks_procfile_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data)
{
	int count = 0;


	count += snprintf(buf+count, bufsize-count, "open count: %d\n", bks_getOpenCount());
	//count += snprintf(buf+count, bufsize-count, "\n");
	//count += snprintf(buf+count, bufsize-count, "\n");

	// either way, tell caller that this is EOF. 
	if (eof) *eof = -1;
	return count;
}



