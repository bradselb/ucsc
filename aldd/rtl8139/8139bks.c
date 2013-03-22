// a basic PCI device driver for a generic
// Realtek RTL8139 PCI Ethernet Card I bought on Amazon.com for $10
// 
// I've closely followed the example set by 8139cp.c


#define DRV_NAME "8139bks"
#define DRV_VERSION "1.0"
#define DEV_NAME "bks%d"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/cache.h>
#include <linux/pci.h>
#include <linux/ioport.h> // IO resource flags
#include <asm/io.h>

#include <asm/uaccess.h>

MODULE_AUTHOR("Brad Selbrede");
MODULE_DESCRIPTION("PCI Driver for Realtek rtl8139 Ethernet card");
MODULE_LICENSE("GPL v2");


// --------------------------------------------------------------------------
// one of the old desktop machines that I use for development has a
// RealTek rtl8100c soldered to the motherboard. This is the primary 
// ethernet NIC and I want the production device driver to be bound to
// that chip (so that I can still use eth0). Luckily, the two devices
// can be differentiated by the subsystem vendor id. The one soldered 
// to the board has subsystem vendor ID == 0x1462 (Micro-Star Int'l).
// My driver will only support the generic Realtek Subsystem VendorID.
static struct pci_device_id rtl8139bks_table[] __devinitdata = {
    {0x10ec, 0x8139, 0x10ec, 0x8139, 0, 0, 0 },
    { 0, 0}
};

MODULE_DEVICE_TABLE(pci, rtl8139bks_table);


// --------------------------------------------------------------------------
// define private data structure
struct rtl8139bks_private {
    struct pci_dev* pdev;
    struct net_device* dev;

    void* __iomem ioaddr; // memory mapped base address of the io registers

    struct net_device_stats stats;
    spinlock_t lock;

    /* Add rtl8139 device specific stuff later */
};

// --------------------------------------------------------------------------
static int __devinit rtl8139bks_probe(struct pci_dev* pdev, const struct pci_device_id* id);
static void __devexit rtl8139bks_remove(struct pci_dev* pdev);

static int rtl8139bks_open(struct net_device* dev);
static int rtl8139bks_stop(struct net_device* dev);
static int rtl8139bks_start_xmit(struct sk_buff* skb, struct net_device* dev);
static struct net_device_stats* rtl8139bks_get_stats(struct net_device* dev);


static struct pci_driver rtl8139bks_driver = {
    .name = DRV_NAME,
    .id_table = rtl8139bks_table,
    .probe = rtl8139bks_probe,
    .remove = __devexit_p(rtl8139bks_remove),

    // PCI suspend and resume routines are not implemented
    //.suspend  = rtl8139bks_suspend,
    //.resume   = rtl8139bks_resume,
};

#ifdef HAVE_NET_DEVICE_OPS
static struct net_device_ops rtl8139bks_netdev_ops = {
    .ndo_open               = rtl8139bks_open,
    .ndo_stop               = rtl8139bks_stop,
    .ndo_get_stats          = rtl8139bks_get_stats,
    .ndo_start_xmit         = rtl8139bks_start_xmit
};
#endif





// **************** Net device ****************************** 

// --------------------------------------------------------------------------
static int rtl8139bks_open(struct net_device* dev)
{
    printk(KERN_INFO "%s rtl8139bks_open()\n", DRV_NAME);
    netif_start_queue(dev); /* transmission queue start */
    return 0;
}

// --------------------------------------------------------------------------
static int rtl8139bks_stop(struct net_device* dev)
{
    printk(KERN_INFO "%s rtl8139bks_stop()\n", DRV_NAME);

    netif_stop_queue(dev); /* transmission queue stop */
    return 0;
}

// --------------------------------------------------------------------------
static int rtl8139bks_start_xmit(struct sk_buff* skb, struct net_device* dev)
{
    printk(KERN_INFO "%s rtl8139bks_start_xmit()\n", DRV_NAME);
    dev_kfree_skb(skb); /* Just free it for now */

    return 0;
}

// --------------------------------------------------------------------------
static struct net_device_stats* rtl8139bks_get_stats(struct net_device* dev)
{
    struct net_device_stats* stats = 0;
    struct rtl8139bks_private* priv = 0;
    printk(KERN_INFO "%s rtl8139bks_get_stats()\n", DRV_NAME);
    priv = netdev_priv(dev);
    stats = &priv->stats;

    return stats;
}



// --------------------------------------------------------------------------
static int __devinit rtl8139bks_probe(struct pci_dev* pdev, const struct pci_device_id* id)
{
    int rc = 0;
    struct net_device* dev;
    struct rtl8139bks_private* priv;
    unsigned long pio_start, pio_end, pio_len, pio_flags;
    unsigned long mmio_start, mmio_end, mmio_len, mmio_flags;
    void* __iomem ioaddr;

    
    // allocate an ethernet device.
    // this also allocates space for the private data and initializes it to zeros. 
    dev = alloc_etherdev(sizeof(struct rtl8139bks_private));
    if (!dev) {
        rc = -ENOMEM;
        goto out;
    }
    SET_NETDEV_DEV(dev, &pdev->dev);
    // initialize some fields in the net device. 
    memcpy(&dev->name[0], DEV_NAME, sizeof DEV_NAME);
    dev->irq = pdev->irq;
    dev->hard_header_len = 14;

    // initialize some fields in the private data
    priv = netdev_priv(dev);
    priv->pdev = pdev;
    priv->dev = dev;
    spin_lock_init(&priv->lock);

    // enable the pci device
    rc = pci_enable_device(pdev);
    if (rc) {
        goto err_out_free_netdev;
    }

    // keep a hook to the private stuff in the pci_device too. 
    pci_set_drvdata(pdev, priv);

    // enable bus master
    pci_set_master(pdev);

    // find out where the io regions are.
    pio_start = pci_resource_start (pdev, 0);
    pio_end = pci_resource_end (pdev, 0);
    pio_len = pci_resource_len (pdev, 0);
    pio_flags = pci_resource_flags (pdev, 0);
    printk(KERN_INFO "%s port-mapped I/O start: %04lx, end: %04lx, length: %lx, flags: %08lx\n", 
                      DRV_NAME, pio_start, pio_end, pio_len, pio_flags);

    mmio_start = pci_resource_start (pdev, 1);
    mmio_end = pci_resource_end (pdev, 1);
    mmio_len = pci_resource_len (pdev, 1);
    mmio_flags = pci_resource_flags (pdev, 1);
    printk(KERN_INFO "%s memory-mapped I/O start: %04lx, end: %04lx, length: %lx, flags: %08lx\n", 
                      DRV_NAME, mmio_start, mmio_end, mmio_len, mmio_flags);


    // make sure that region #1 is memory mapped.
    if (!(mmio_flags & IORESOURCE_MEM)) {
        printk(KERN_WARNING "%s IO resource region #1 is not memory mapped - aborting now.\n", DRV_NAME);
        rc = -ENODEV;
        goto err_out_clear_master;
    }


    // take ownership of all the io regions (in this case there are only two)
    rc = pci_request_regions(pdev, DRV_NAME);
    if (rc) {
        goto err_out_clear_master;
    }

    // ask kernel to setup page tables for the mmio region.
    ioaddr = pci_iomap(pdev, 1, 0);
    if (!ioaddr) {
        rc = -EIO;
        goto err_out_release_regions;
    }

    // save off the base address of the io registers
    dev->base_addr = (long)ioaddr;
    priv->ioaddr = ioaddr;

    // configure 32bit DMA.
    rc = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
    if (rc) {
        goto err_out_iounmap;
    }

//  rc = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
//  if (rc) {
//      goto err_out_iounmap;
//  }

    memset(&dev->broadcast[0], 255, 6);
    memcpy_fromio(&dev->dev_addr[0], ioaddr, 6);

#ifdef HAVE_NET_DEVICE_OPS
    dev->netdev_ops = &rtl8139bks_netdev_ops;
#else
    dev->open = rtl8139bks_open;
    dev->stop = rtl8139bks_stop;
    dev->hard_start_xmit = rtl8139bks_start_xmit;
    dev->get_stats = rtl8139bks_get_stats;
#endif

    rc = register_netdev(dev);
    if (rc) {
        goto err_out_iounmap;
    }

    rc = 0;
    goto out;

err_out_iounmap:
    pci_iounmap(pdev, ioaddr);

err_out_release_regions:
    pci_release_regions(pdev);

//err_out_mwi:
//  pci_clear_mwi(pdev);

err_out_clear_master:
    pci_clear_master(pdev);

//err_out_disable:
    pci_disable_device(pdev);

err_out_free_netdev:
    free_netdev(dev);

out:
    return rc;
}

// --------------------------------------------------------------------------
//
//  * Get address of netdevice, device private structures and ioaddr
//  * Unregister and free netdevice
//  * Unmap the device MMIO region. Also set: priv->mmio_addr = NULL
//  * Release the ownership of IO memory region
//  * call: pci_set_drvdata(pdev, NULL)
//  * Disable PCI device
static void __devexit rtl8139bks_remove(struct pci_dev* pdev)
{
    struct net_device* dev = 0;
    struct rtl8139bks_private* priv = 0;
    void* __iomem ioaddr = 0;

    printk(KERN_INFO "%s rtl8139bks_remove()\n", DRV_NAME);

    priv = (struct rtl8139bks_private*)pci_get_drvdata(pdev);
    if (priv) {
        dev = priv->dev;
        ioaddr = priv->ioaddr;
        pci_iounmap(pdev, ioaddr);
        priv->ioaddr = 0; // prophy.
    }
        
    if (dev) {
        unregister_netdev(dev);
    }

    pci_release_regions(pdev);
    pci_clear_master(pdev);
    pci_set_drvdata(pdev,0);
    pci_disable_device(pdev);

    if (dev) {
        free_netdev(dev);
    }
}



// --------------------------------------------------------------------------
static int __init pci_rtl8139bks_init(void)
{
    int rc = 0;
    printk(KERN_INFO "%s rtl8139bks_init()\n", DRV_NAME);
    rc = pci_register_driver(&rtl8139bks_driver);
    return rc;
}

// --------------------------------------------------------------------------
static void __exit pci_rtl8139bks_exit(void)
{
    printk(KERN_INFO "%s rtl8139bks_exit()\n", DRV_NAME);
    pci_unregister_driver(&rtl8139bks_driver);
}

module_init(pci_rtl8139bks_init);
module_exit(pci_rtl8139bks_exit);

