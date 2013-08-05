/* 
 * a linux kernel module demonstrating the use of the proc filesystem 
 * this module creates a proc file entry that provides information about
 * about the location and number of the page struct arrary. 
 *
 * by Brad Selbrede, 
 * for UCSC-ext Linux Kernel Architecture class, Winter 2012
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mm_types.h>  // struct page
#include <linux/mm.h>
//#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");


static const char* BKS_PROC_DIR_NAME = "bks";

static struct proc_dir_entry* bks_proc_dir_entry;


static int bks_proc_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data)
{
	int count = 0;
	struct page* page = 0;
	int i;

	// some counters
	int nonzero_freelist = 0;
	int nonzero_flags = 0;
	int nonzero_counters = 0;

	printk(KERN_INFO "BKS: bks_proc_read(),  offset: %lu, bufsize: %d, eof: %p, data: %p\n", offset, bufsize, eof, data);

	for (i=0; i<max_mapnr; ++i) {
		page = pfn_to_page(i);
		if (!page) continue;
		if (page->freelist) ++nonzero_freelist;
		if (page->flags) ++nonzero_flags;
		if (page->counters) ++nonzero_counters;
	}

	if ( 0 < offset ) {
		count = 0;
	} else {
		count = snprintf(buf, bufsize, "virtual address of mem_map is: 0x%p, "
					"physical address is: 0x%08x, page_count: %lu (%luMB)\n",
					mem_map, virt_to_phys(mem_map), max_mapnr, (max_mapnr >> 8));
		count += snprintf(buf+count, bufsize-count, "number of pages with nonzero flags: %d\n", nonzero_flags);
		count += snprintf(buf+count, bufsize-count, "number of pages with nonzero counter: %d\n", nonzero_counters);
	}
	// either way, tell caller that this is EOF. 
	if (eof) *eof = -1;

	return count;
}


static int __init bks_proc_init(void)
{
	int rc = 0;

	printk(KERN_INFO "BKS: bks_proc_init()\n");

	bks_proc_dir_entry = create_proc_entry(BKS_PROC_DIR_NAME, 0 , NULL);
	if (!bks_proc_dir_entry) {
		printk(KERN_ERR "BKS: unable to register /proc/%s\n", BKS_PROC_DIR_NAME);
		rc = -ENOMEM;
	} else {
		printk(KERN_INFO "BKS: successfully registered /proc/%s\n", BKS_PROC_DIR_NAME);
		bks_proc_dir_entry->read_proc = bks_proc_read;
		rc = 0;
	}
	return rc;
}


static void __exit bks_proc_exit(void)
{
	printk(KERN_INFO "BKS: bks_proc_exit()\n");
	remove_proc_entry(BKS_PROC_DIR_NAME , NULL);
}



module_init(bks_proc_init);
module_exit(bks_proc_exit);



