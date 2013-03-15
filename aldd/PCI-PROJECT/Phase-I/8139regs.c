//-------------------------------------------------------------------
//	8139regs.c
//
//	This module creates a pseudo-file (named '/proc/8139regs')
//	which allows users to view the contents of the significant
//	read/write registers in the RTL8139 programming interface.
//
//	NOTE: Written and tested with Linux kernel version 2.6.36.
//
//	programmer: ALLAN CRUSE
//	written on: 09 NOV 2010
//-------------------------------------------------------------------

#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>	// for create_proc_read_entry()
#include <linux/pci.h>		// for pci_get_device() 

#define VENDOR_ID  0x10EC	// RealTek Semiconductor Corp
#define	DEVICE_ID  0x8139 	// RTL-8139 Network Processor

char modname[] = "8139regs";	// for displaying module name
unsigned long mmio_base;	// address-space of registers
unsigned long mmio_size;	// size of the register-space
void*	io;			// virtual address of ioremap
char legend[] = "RealTek-8139 Network Controller register-values";

int my_proc(char* buf, char** s, off_t off, int bufsz, int* eof, void* data)
{
    int	i, len = 0;

    len += sprintf(buf+len, "\n\n %13s %s \n", " ", legend);

    len += sprintf(buf+len, "\n      ");
    len += sprintf(buf+len, " CR=%02X   ",  readb(io + 0x37));
    len += sprintf(buf+len, " TCR=%08X  ", readl(io + 0x40));
    len += sprintf(buf+len, " RCR=%08X  ", readl(io + 0x44));
    len += sprintf(buf+len, "   MAC=");
    for (i = 0; i < 6; i++) {
        len += sprintf(buf+len, "%02X", readb(io + i));
        len += sprintf(buf+len, "%c", (i<5) ? ':' : ' ');
    }
    len += sprintf(buf+len, "\n");

    len += sprintf(buf+len, "\n      ");
    len += sprintf(buf+len, "  TSD0=%08X ", readl(io + 0x10));
    len += sprintf(buf+len, "  TSD1=%08X ", readl(io + 0x14));
    len += sprintf(buf+len, "  TSD2=%08X ", readl(io + 0x18));
    len += sprintf(buf+len, "  TSD3=%08X ", readl(io + 0x1C));

    len += sprintf(buf+len, "\n      ");
    len += sprintf(buf+len, " TSAD0=%08X ", readl(io + 0x20));
    len += sprintf(buf+len, " TSAD1=%08X ", readl(io + 0x24));
    len += sprintf(buf+len, " TSAD2=%08X ", readl(io + 0x28));
    len += sprintf(buf+len, " TSAD3=%08X ", readl(io + 0x2C));
    len += sprintf(buf+len, "\n");

    len += sprintf(buf+len, "\n      ");
    len += sprintf(buf+len, " RBSTART=%08X ", readl(io + 0x30));
    len += sprintf(buf+len, " CAPR=%04X ", readw(io + 0x38));
    len += sprintf(buf+len, " CABR=%04X ", readw(io + 0x3A));
    len += sprintf(buf+len, "   ");
    len += sprintf(buf+len, " ERBCR=%04X ", readw(io + 0x34));
    len += sprintf(buf+len, " ERSR=%02X ",  readb(io + 0x36));
    len += sprintf(buf+len, "\n");

    len += sprintf(buf+len, "\n      ");
    len += sprintf(buf+len, " IMR=%04X  ", readw(io + 0x3C));
    len += sprintf(buf+len, " MULINT=%04X ", readw(io + 0x5C));
    len += sprintf(buf+len, "   TSAD=%04X   ", readw(io + 0x60));
    len += sprintf(buf+len, " REC=%02X ",  readb(io + 0x72));
    len += sprintf(buf+len, " DIS=%02X ",  readb(io + 0x6C));
    len += sprintf(buf+len, " FCSC=%02X ",  readb(io + 0x6E));
    len += sprintf(buf+len, "\n");

    len += sprintf(buf+len, "\n\n");
    return	len;
}

int init_module(void)
{
    struct pci_dev*	devp = NULL;

    printk("<1>\nInstalling \'%s\' module\n", modname);

    devp = pci_get_device(VENDOR_ID, DEVICE_ID, devp);
    if (!devp) {
        return -ENODEV;
    }

    mmio_base = pci_resource_start(devp, 1);
    mmio_size = pci_resource_len(devp, 1);
    printk("  RealTek 8139 Network Interface:");
    printk("  %08lX-%08lX \n", mmio_base, mmio_base+mmio_size);

    io = ioremap(mmio_base, mmio_size);
    if (!io) {
        return -ENOSPC;
    }

    create_proc_read_entry(modname, 0, NULL, my_proc, NULL);
    return	0;  // SUCCESS
}

void cleanup_module(void)
{
    remove_proc_entry(modname, NULL);
    iounmap(io);
    printk("<1>Removing \'%s\' module\n", modname);
}

MODULE_LICENSE("GPL");
