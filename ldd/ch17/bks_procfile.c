#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/netdevice.h>

// -------------------------------------------------------------------------
// globals
static const char* BKS_PROC_FILE_NAME = "bks";
static struct proc_dir_entry* bks_proc_dir_entry;

// -------------------------------------------------------------------------
static int bks_proc_read(char *buf, char **start, off_t offset, int bufsize, int *eof, void *data)
{
	int count = 0;
	struct net_device* ndev = 0;


	if ( 0 < offset ) {
		count = 0;
	} else if (0 == (ndev = dev_get_by_name(&init_net, "eth0"))) {
		printk(KERN_INFO "dev_get_by_name() failed.\n");
	} else {
		int i;
		count += snprintf(buf+count, bufsize-count, "Brad Selbrede - Chapter 17\n");
		count += snprintf(buf+count, bufsize-count, "name: %s\n", ndev->name);

		count += snprintf(buf+count, bufsize-count, "mac address: ");
		for (i = 0; i < ndev->addr_len; ++i) {
			count += snprintf(buf+count, bufsize-count, "%02x:", ndev->perm_addr[i]);
		}
		count += snprintf(buf+count, bufsize-count, "\n");
		
		//count += snprintf(buf+count, bufsize-count, "mem_start: 0x%08lx\n", ndev->mem_start);
		//count += snprintf(buf+count, bufsize-count, "mem_end: 0x%08lx\n", ndev->mem_end);
		count += snprintf(buf+count, bufsize-count, "base_addr: 0x%08lx\n", ndev->base_addr);
		count += snprintf(buf+count, bufsize-count, "irq: %d\n", ndev->irq);
		count += snprintf(buf+count, bufsize-count, "dma: %d\n", ndev->dma);
		count += snprintf(buf+count, bufsize-count, "features: 0x%08lx\n", ndev->features);

		count += snprintf(buf+count, bufsize-count, "ifindex: %d \n", ndev->ifindex);
		//count += snprintf(buf+count, bufsize-count, "perm_addr: %s\n",ndev->perm_addr);
		count += snprintf(buf+count, bufsize-count, "addr_len: %d\n",ndev->addr_len);
		//count += snprintf(buf+count, bufsize-count, "dev_addr: %s\n", ndev->dev_addr);
		//count += snprintf(buf+count, bufsize-count, "x: %d\n", x);
		//count += snprintf(buf+count, bufsize-count, "\n");
		//count += snprintf(buf+count, bufsize-count, "\n");
		dev_put(ndev);
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


