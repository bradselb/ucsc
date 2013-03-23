// a basic PCI device driver for a generic
// Realtek RTL8139 PCI Ethernet Card I bought on Amazon.com for $10
// 
// I've taken considerable inspiration from 8139too.c and 8139cp.c


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

#include "8139bks.h" // register definitions

MODULE_AUTHOR("Brad Selbrede");
MODULE_DESCRIPTION("PCI Driver for Realtek rtl8139 Ethernet card");
MODULE_LICENSE("GPL v2");


// --------------------------------------------------------------------------
// one of the old desktop machines that I use for development has a
// RealTek rtl8100c soldered to the motherboard. This is the primary 
// ethernet NIC and I want the device driver '8139too' to be bound to
// that chip so that I can still use eth0. Luckily, the two devices
// can be differentiated by the subsystem vendor id. The one soldered
// to the board has subsystem vendor ID == 0x1462 (Micro-Star Int'l).
// I've hacked up the sources for the 8139too driver so that it only 
// supports the 8139 chip with the subsystem vendor id, 0x1462. 
// My driver will only support the generic Realtek Subsystem VendorID.
static struct pci_device_id rtl8139bks_table[] __devinitdata = {
    {0x10ec, 0x8139, 0x10ec, 0x8139, 0, 0, 0 },
    { 0, 0}
};

MODULE_DEVICE_TABLE(pci, rtl8139bks_table);



// --------------------------------------------------------------------------
// the datasheet is a little sketchy on the necessary receive buffer size
// make it big enough that we need not worry about it too much. 
#define RX_BUF_SIZE (65536)
#define RX_BUF_TOT_SIZE (65536)
#define NR_OF_TX_DESCRIPTORS (4)
#define MAX_ETH_FRAME_SIZE	1536
#define TX_BUF_SIZE	MAX_ETH_FRAME_SIZE
#define TX_BUF_TOT_SIZE	(TX_BUF_SIZE * NR_OF_TX_DESCRIPTORS )

// --------------------------------------------------------------------------
// define private data structure
struct rtl8139bks_private {
    struct pci_dev* pdev;
    struct net_device* ndev;

    void* __iomem ioaddr; // base address of the memory mapped io registers

    unsigned int tx_cur; // an index into the tx buffer. reflects current position in the buffer? or which bufer?
    unsigned int tx_dirty; // ?? 
    unsigned char* tx_buf[NR_OF_TX_DESCRIPTORS]; // an array of dynamically allocated buffers.
    unsigned char* tx_bufs;
    dma_addr_t tx_bufs_dma;

    void* rx_ring;
    unsigned int rx_cur;
    dma_addr_t rx_ring_dma;

    struct net_device_stats stats;

    // spin lock protects access to chip's registers. 
    spinlock_t lock;
};

// --------------------------------------------------------------------------
// forward declarations of functions
static int __devinit rtl8139bks_probe(struct pci_dev* pdev, const struct pci_device_id* id);
static void __devexit rtl8139bks_remove(struct pci_dev* pdev);

static irqreturn_t rtl8139bks_interrupt(int irq, void* dev_id);

static int rtl8139bks_open(struct net_device* net_dev);
static int rtl8139bks_stop(struct net_device* net_dev);
static int rtl8139bks_start_xmit(struct sk_buff* skb, struct net_device* net_dev);
static struct net_device_stats* rtl8139bks_get_stats(struct net_device* net_dev);


static int rtl8139bks_init_hardware(struct rtl8139bks_private*);


// --------------------------------------------------------------------------
static struct pci_driver rtl8139bks_driver = {
    .name = DRV_NAME,
    .id_table = rtl8139bks_table,
    .probe = rtl8139bks_probe,
    .remove = __devexit_p(rtl8139bks_remove),
    //.suspend  = rtl8139bks_suspend,
    //.resume   = rtl8139bks_resume,
};

// --------------------------------------------------------------------------
#ifdef HAVE_NET_DEVICE_OPS
static struct net_device_ops rtl8139bks_netdev_ops = {
    .ndo_open = rtl8139bks_open,
    .ndo_stop = rtl8139bks_stop,
    .ndo_get_stats = rtl8139bks_get_stats,
    .ndo_start_xmit = rtl8139bks_start_xmit
};
#endif

// --------------------------------------------------------------------------
// this global variable is a bit of a hack to address the following issue...
// we are sharing interrupts. how do we know that this interrupt
// is for us? How do we know that our device is the one interrupting?
// we're supposed to compare the dev_id passed as an argument to those 
// associated with our device....but we never save those off anywhere.
// for now, just assume that this device is only associated with ONE
// physiscal device in the system. It would be better to keep a list. 
static void* rtl8139bks_interrupt_dev_id = 0;

// --------------------------------------------------------------------------
// the interrupt service function. 
irqreturn_t rtl8139bks_interrupt(int irq, void* dev_id)
{
    int handled = 0;
    struct net_device* net_dev = (struct net_device*)dev_id;
    struct rtl8139bks_private* priv = netdev_priv(net_dev);
    unsigned int intr_stat = 0;
    void* __iomem iobase = priv->ioaddr;

    printk(KERN_INFO "%s rtl8139bks_interrupt(), irq: %d, dev_id: %p\n", DRV_NAME, irq, dev_id);

    if (dev_id && (dev_id == rtl8139bks_interrupt_dev_id) && priv) {
        // read the interrupt status register
        
        spin_lock(&priv->lock);
        intr_stat = ioread16(iobase + IntrStatus);
        spin_unlock(&priv->lock);

        // use status to determine reason for interrupt.

        printk(KERN_INFO "%s interrupt status: 0x%04x\n", DRV_NAME, intr_stat);
        
        handled = 1;

    } else {
        handled = 0;
    }

    return IRQ_RETVAL(handled);
}



// **************** net device ops ****************************** 

// --------------------------------------------------------------------------
int rtl8139bks_open(struct net_device* ndev)
{
    int rc = 0;
    struct rtl8139bks_private* priv = netdev_priv(ndev);
    //void* __iomem ioaddr = priv->ioaddr;

    printk(KERN_INFO "%s rtl8139bks_open()\n", DRV_NAME);

    // register our interrupt handler.
    rc = request_irq(ndev->irq, rtl8139bks_interrupt, IRQF_SHARED, ndev->name, ndev);
    if (rc != 0) {
        goto exit; // bail out with this error code.
    }

    // HACK. See discussion in comment above interrupt() function.
    rtl8139bks_interrupt_dev_id = ndev;

    // allocate DMA buffers for both receive and transmit
    // void* pci_alloc_consistent(struct pci_dev*, size_t, dma_add_t*);
    // is a convenient wrapper around...
    // void* dma_alloc_coherent(struct device*, size_t, dma_addr_t*, gfp_t);
    priv->tx_bufs = pci_alloc_consistent(priv->pdev, TX_BUF_TOT_SIZE, &priv->tx_bufs_dma);
    if (!priv->tx_bufs) {
        rc = -ENOMEM;
        goto err_out_free_irq;
    }
    priv->rx_ring = pci_alloc_consistent(priv->pdev, RX_BUF_TOT_SIZE, &priv->rx_ring_dma);
    if (!priv->rx_ring) {
        rc = -ENOMEM;
        goto err_out_free_dma;
    }

    // initialize all of the rest of the private data
    // esecially the buffer accounting.
    priv->rx_cur = 0;
    priv->tx_cur = 0;
    priv->tx_dirty = 0;
    for (int i=0; i<NR_OF_TX_DESCRIPTORS; ++i) {
        priv->tx_buf[i] = &priv->tx_bufs[i * TX_BUF_SIZE];
    }

    rtl8139bks_init_hardware(priv);

    netif_start_queue(ndev);
    rc = 0; // if we get this far all is well.
    goto exit; // successfully.

err_out_free_dma:
    if (priv->rx_ring) {
        pci_free_consistent(priv->pdev, RX_BUF_TOT_SIZE, priv->rx_ring, priv->rx_ring_dma);
    }
    if (priv->tx_bufs) {
        pci_free_consistent(priv->pdev, TX_BUF_TOT_SIZE, priv->tx_bufs, priv->tx_bufs_dma);
    }

err_out_free_irq:
	free_irq(ndev->irq, ndev);

exit:
    return rc;
}

// --------------------------------------------------------------------------
int rtl8139bks_stop(struct net_device* ndev)
{
    int rc = 0;
    struct rtl8139bks_private* priv = netdev_priv(ndev);
    //void* __iomem ioaddr = priv->ioaddr;

    printk(KERN_INFO "%s rtl8139bks_stop()\n", DRV_NAME);

    // stop transmission
    netif_stop_queue(ndev);

    // command chip to stop DMA transfers

    // disable interrupts

    // update statistics

    // free dma buffers
    if (priv->rx_ring) {
        pci_free_consistent(priv->pdev, RX_BUF_TOT_SIZE, priv->rx_ring, priv->rx_ring_dma);
    }

    if (priv->tx_bufs) {
        pci_free_consistent(priv->pdev, TX_BUF_TOT_SIZE, priv->tx_bufs, priv->tx_bufs_dma);
    }

    // free the irq
	free_irq(ndev->irq, ndev);

    // re-set transmit accounting.
    priv->tx_cur = 0;
    priv->tx_dirty = 0;


    return rc;
}

// --------------------------------------------------------------------------
int rtl8139bks_start_xmit(struct sk_buff* skb, struct net_device* dev)
{
    printk(KERN_INFO "%s rtl8139bks_start_xmit()\n", DRV_NAME);
    dev_kfree_skb(skb); /* Just free it for now */

    return 0;
}

// --------------------------------------------------------------------------
struct net_device_stats* rtl8139bks_get_stats(struct net_device* dev)
{
    struct net_device_stats* stats = 0;
    struct rtl8139bks_private* priv = 0;
    printk(KERN_INFO "%s rtl8139bks_get_stats()\n", DRV_NAME);
    priv = netdev_priv(dev);
    stats = &priv->stats;

    return stats;
}

// ************************ private functions ****************************
// --------------------------------------------------------------------------
static void rtl8139_reset_chip (struct rtl8139bks_private* priv)
{
    void* __iomem iobase = priv->ioaddr;

	// command a soft reset
    iowrite8(CmdSoftReset, iobase + ChipCmd);

    // wait here until the chip says it has completed the reset
	for (int i = 1000; 0 < i; --i) {
		barrier();
		if (0 == (ioread8(iobase + ChipCmd) & CmdSoftReset)) break;
		udelay(10);
	}
}



// --------------------------------------------------------------------------
static int rtl8139bks_init_hardware(struct rtl8139bks_private* priv)
{
    int rc = 0;

    rtl8139_reset_chip(priv);

    return rc;
}




// **************** PCI device ****************************** 
// --------------------------------------------------------------------------
int __devinit rtl8139bks_probe(struct pci_dev* pdev, const struct pci_device_id* id)
{
    int rc = 0;
    struct net_device* ndev;
    struct rtl8139bks_private* priv;
    unsigned long pio_start, pio_end, pio_len, pio_flags;
    unsigned long mmio_start, mmio_end, mmio_len, mmio_flags;
    void* __iomem ioaddr;

    
    // allocate an ethernet device.
    // this also allocates space for the private data and initializes it to zeros. 
    ndev = alloc_etherdev(sizeof(struct rtl8139bks_private));
    if (!ndev) {
        rc = -ENOMEM;
        goto out;
    }
    SET_NETDEV_DEV(ndev, &pdev->dev);
    // initialize some fields in the net device. 
    memcpy(&ndev->name[0], DEV_NAME, sizeof DEV_NAME);
    ndev->irq = pdev->irq;
    ndev->hard_header_len = 14;

    // initialize some fields in the private data
    priv = netdev_priv(ndev);
    priv->pdev = pdev;
    priv->ndev = ndev;
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
    printk(KERN_INFO "%s port-mapped I/O start: 0x%04lx, end: 0x%04lx, length: 0x%lx, flags: 0x%08lx\n", 
                      DRV_NAME, pio_start, pio_end, pio_len, pio_flags);

    mmio_start = pci_resource_start (pdev, 1);
    mmio_end = pci_resource_end (pdev, 1);
    mmio_len = pci_resource_len (pdev, 1);
    mmio_flags = pci_resource_flags (pdev, 1);
    printk(KERN_INFO "%s memory-mapped I/O start: 0x%04lx, end: 0x%04lx, length: 0x%lx, flags: 0x%08lx\n", 
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
    ndev->base_addr = (long)ioaddr;
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

    memset(&ndev->broadcast[0], 255, 6);
    memcpy_fromio(&ndev->dev_addr[0], ioaddr, 6);

#ifdef HAVE_NET_DEVICE_OPS
    ndev->netdev_ops = &rtl8139bks_netdev_ops;
#else
    ndev->open = rtl8139bks_open;
    ndev->stop = rtl8139bks_stop;
    ndev->hard_start_xmit = rtl8139bks_start_xmit;
    ndev->get_stats = rtl8139bks_get_stats;
#endif

    rc = register_netdev(ndev);
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
    free_netdev(ndev);

out:
    return rc;
}

// --------------------------------------------------------------------------
//
// Get address of netdevice, device private structures and ioaddr
// Unregister and free netdevice
// Unmap the device MMIO region. Also set: priv->mmio_addr = NULL
// Release the ownership of IO memory region
// call: pci_set_drvdata(pdev, NULL)
// Disable PCI device
void __devexit rtl8139bks_remove(struct pci_dev* pdev)
{
    struct net_device* ndev = 0;
    struct rtl8139bks_private* priv = 0;
    void* __iomem ioaddr = 0;

    printk(KERN_INFO "%s rtl8139bks_remove()\n", DRV_NAME);

    priv = (struct rtl8139bks_private*)pci_get_drvdata(pdev);
    if (priv) {
        ndev = priv->ndev;
        ioaddr = priv->ioaddr;
        pci_iounmap(pdev, ioaddr);
        priv->ioaddr = 0; // prophy.
    }
        
    if (ndev) {
        unregister_netdev(ndev);
    }

    pci_release_regions(pdev);
    pci_clear_master(pdev);
    pci_set_drvdata(pdev,0);
    pci_disable_device(pdev);

    if (ndev) {
        free_netdev(ndev);
    }
}


// **************** basic driver ******************************
// --------------------------------------------------------------------------
int __init pci_rtl8139bks_init(void)
{
    int rc = 0;
    printk(KERN_INFO "%s rtl8139bks_init()\n", DRV_NAME);
    rc = pci_register_driver(&rtl8139bks_driver);
    return rc;
}

// --------------------------------------------------------------------------
void __exit pci_rtl8139bks_exit(void)
{
    printk(KERN_INFO "%s rtl8139bks_exit()\n", DRV_NAME);
    pci_unregister_driver(&rtl8139bks_driver);
}

module_init(pci_rtl8139bks_init);
module_exit(pci_rtl8139bks_exit);

