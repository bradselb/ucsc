#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
//#include <linux/memory.h>
//#include <linux/mm_types.h>
//#include <linux/mm.h>

#include <linux/jiffies.h>
#include <linux/timex.h>
//#include <asm/msr.h> // rdtsc()

// -------------------------------------------------------------------------
// globals
static const char* BKS_PROC_FILE_NAME = "bks";
static struct proc_dir_entry* bks_proc_dir_entry;



// -------------------------------------------------------------------------
static int bks_proc_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data)
{
	int count = 0;
	long j0, j1; // jiffies.
	long diff;
	cycles_t c0, c1; // CPU clock cycles
	//unsigned long long c0, c1;
	unsigned long elapsed_jiffies;
	unsigned long elapsed_clocks;
	unsigned long clocks_per_jiffie;
	unsigned long cpu_clock_freq = 0; // in Hz

	if ( 0 < offset ) {
		count = 0;
	} else {
		count = snprintf(buf, bufsize, "Hello World!\n");
		j0 = jiffies;
		c0 = get_cycles();
		//rdtscll(c0);
		j1 = jiffies;
		while ( (diff =  1000 * (j1 - j0) / HZ) < 500) {
			cpu_relax();
			j1 = jiffies;
		}
		c1 = get_cycles();
		//rdtscll(c1);

		elapsed_clocks = (c1 - c0);
		elapsed_jiffies =  (j1 - j0);
		clocks_per_jiffie = elapsed_clocks / elapsed_jiffies;
		cpu_clock_freq = (unsigned long)HZ * clocks_per_jiffie ;

		count += snprintf(buf+count, bufsize-count, "delay: %ld (mS)\n", diff);
		count += snprintf(buf+count, bufsize-count, "elapsed clocks: %lu\n", elapsed_clocks);
		count += snprintf(buf+count, bufsize-count, "elapsed jiffies: %lu\n", elapsed_jiffies);
		count += snprintf(buf+count, bufsize-count, "clocks per jiffie: %lu\n", clocks_per_jiffie);
		count += snprintf(buf+count, bufsize-count, "jiffies per second %d\n", HZ);

		count += snprintf(buf+count, bufsize-count, "CPU Clock frequency: %lu (Hz)\n", cpu_clock_freq);
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

