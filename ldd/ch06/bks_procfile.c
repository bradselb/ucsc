#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/memory.h>
#include <linux/mm_types.h>
#include <linux/mm.h>

#include "bks_fileops.h" // buffer length and size
#include "bks_opencount.h"

// -------------------------------------------------------------------------
// forward decl's
static int bks_procfile_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data);

// -------------------------------------------------------------------------
// globals
static const char* BKS_PROC_DIR_NAME = "bks";

// strlen(BKS_PROC_DIR_NAME) + strlen("01") + 1 < BKS_MAX_PROCFILE_NAME_LENGTH 
#define BKS_MAX_PROCFILE_NAME_LENGTH 8
#define BKS_MAX_PROC_FILES 8
static int actual_proc_files;
static int private_data[BKS_MAX_PROC_FILES];

static int bks_group = 42; // default to the answer to the universe 
module_param(bks_group, int, 0);
MODULE_PARM_DESC(bks_group, "group number");



// -------------------------------------------------------------------------
int __init bks_createProcDirEntry(int count) // really, "entries"
{
	int rc = 0;
	int i, n;
	char name[BKS_MAX_PROCFILE_NAME_LENGTH];
	struct proc_dir_entry* entry = 0;

	printk(KERN_INFO "[bks] bks_createProcDirEntry(), count: %d\n", count);

	n = (count < BKS_MAX_PROC_FILES ? count : BKS_MAX_PROC_FILES);
	for (i=0; i<n; ++i) {
		private_data[i] = i;
		snprintf(name, sizeof name, "%s%02d", BKS_PROC_DIR_NAME, i);
		entry = create_proc_read_entry(name, 0, 0, bks_procfile_read, &private_data[i]);
		if (!entry) {
			printk(KERN_ERR "[bks] unable to create dir /proc/%s\n", name);
			rc = -ENOMEM;
			break;
		}
	}
	// on success, i == n but even if we break out due to a failure, the zero
	// based index, i, of first failure == number successfully created entries.
	actual_proc_files = i;

	return rc;
}


// -------------------------------------------------------------------------
void __exit bks_removeProcDirEntry(void)
{
	int i, n;
	char name[BKS_MAX_PROCFILE_NAME_LENGTH];

	printk(KERN_INFO "[bks] bks_removeProcDirEntry()\n");

	n = actual_proc_files;
	for (i=0; i<n; ++i) {
		snprintf(name, sizeof name, "%s%02d", BKS_PROC_DIR_NAME, i);
		remove_proc_entry(name , 0);
	}

}




// -------------------------------------------------------------------------
static int bks_procfile_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data)
{
	int count = 0;
	int nr = 0;

	if ( 0 < offset ) {
		count = 0;
	} else {
		nr = *(int*)data;

		count = snprintf(buf, bufsize, "Group number: %d\n", bks_group);
		count += snprintf(buf+count, bufsize-count, "Group members: Brad Selbrede\n");
		count += snprintf(buf+count, bufsize-count, "minor number: %d\n", nr);
		count += snprintf(buf+count, bufsize-count, "buffer size: %d\n", bks_getBufferSize(nr));
		count += snprintf(buf+count, bufsize-count, "buffer content length: %d\n", bks_getBufferContentLength(nr));
		count += snprintf(buf+count, bufsize-count, "open count: %d\n", bks_getOpenCount());
		//count += snprintf(buf+count, bufsize-count, "\n");
		//count += snprintf(buf+count, bufsize-count, "\n");
	}

	// either way, tell caller that this is EOF. 
	
	if (eof) *eof = -1;
	return count;
}



