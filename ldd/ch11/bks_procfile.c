#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
//#include <linux/memory.h>
//#include <linux/mm_types.h>
//#include <linux/mm.h>
//#include <linux/jiffies.h>
//#include <linux/timex.h>
//#include <asm/msr.h> // rdtsc()
#include <asm/byteorder.h>

// -------------------------------------------------------------------------
// globals
static const char* BKS_PROC_FILE_NAME = "bks";
static struct proc_dir_entry* bks_proc_dir_entry;

// -------------------------------------------------------------------------
static int isLittleEndian(void)
{
	volatile unsigned int k = 0x76543210;
	int v = (cpu_to_le32(k) == k);
	printk(KERN_INFO "k: 0x%08x, cpu_to_le(k): 0x%08x, cpu_to_be(k): 0x%08x\n", k, cpu_to_le32(k), cpu_to_be32(k));
	return v;
}

// -------------------------------------------------------------------------
static int isCharSigned(void)
{
	char a = 0x7f;
	char b = 0x80;
	int v = (b < a);
	return v;
}


// -------------------------------------------------------------------------
static int bks_proc_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data)
{
	int count = 0;

	if ( 0 < offset ) {
		count = 0;
	} else {
		count += snprintf(buf+count, bufsize-count, "Brad Selbrede - Chapter 11\n");
		count += snprintf(buf+count, bufsize-count, "this is a %s endian machine.\n", (isLittleEndian() ? "little" : "big"));
		count += snprintf(buf+count, bufsize-count, "page size is: %lu\n", PAGE_SIZE);
		count += snprintf(buf+count, bufsize-count, "size of u8: %u, u16: %u, u32: %u, u64: %u\n", sizeof(u8), sizeof(u16), sizeof(u32), sizeof(u64));
		count += snprintf(buf+count, bufsize-count, "size of s8: %u, s16: %u, s32: %u, s64: %u\n", sizeof(s8), sizeof(s16), sizeof(s32), sizeof(s64));
		count += snprintf(buf+count, bufsize-count, "size of int8_t: %u, int16_t: %u, int32_t: %u, int64_t: %u\n", sizeof(int8_t), sizeof(int16_t), sizeof(int32_t), sizeof(int64_t));
		count += snprintf(buf+count, bufsize-count, "char is %ssigned.\n", (isCharSigned() ? "": "un"));
		//count += snprintf(buf+count, bufsize-count, "\n", );
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

