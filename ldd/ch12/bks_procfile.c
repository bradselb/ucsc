#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>

// -------------------------------------------------------------------------
// globals
static const char* BKS_PROC_FILE_NAME = "bks";
static struct proc_dir_entry* bks_proc_dir_entry;
static int enumeratePciBus(char* buf, int bufsize); // return char count. minus --> error.

// -------------------------------------------------------------------------
static int bks_proc_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data)
{
	int count = 0;

	if ( 0 < offset ) {
		count = 0;
	} else {
		count += snprintf(buf+count, bufsize-count, "Brad Selbrede - Chapter 12\n");
		count += enumeratePciBus(buf+count, bufsize-count);
		count += snprintf(buf+count, bufsize-count, "\n");
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

// -------------------------------------------------------------------------
int enumeratePciBus(char* buf, int bufsize)
{
	int count = 0;
	unsigned char v;
	struct pci_dev* pcidev = 0;


	//while (0 != (pcidev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, pcidev))) 
	for_each_pci_dev(pcidev) {
		pci_read_config_byte(pcidev, PCI_VENDOR_ID, &v);
		count += snprintf(buf+count, bufsize-count, "vendor id: %02x\n", v);
		pci_read_config_byte(pcidev, PCI_DEVICE_ID, &v);
		count += snprintf(buf+count, bufsize-count, "device id: %02x\n", v);
		pci_read_config_byte(pcidev, PCI_INTERRUPT_LINE, &v);
		count += snprintf(buf+count, bufsize-count, "interrupt: %02x\n", v);
		pci_read_config_byte(pcidev, PCI_LATENCY_TIMER, &v);
		count += snprintf(buf+count, bufsize-count, "latency timer: %02x\n", v);
		pci_read_config_byte(pcidev, PCI_COMMAND, &v);
		count += snprintf(buf+count, bufsize-count, "command: %02x\n", v);
	} //while

	return count;
}

