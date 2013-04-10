// a basic PCI device driver for a generic
// Realtek RTL8139 Ethernet PCI Card based upon
// 
// I've taken considerable inspiration from 8139too.c and 8139cp.c
// and to a lesser extent, I've also gleened ideas from b44.c and 
// the various Intel E1000 drivers.

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
MODULE_DESCRIPTION("Realtek RTL8139 Fast Ethernet controller");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL v2");


// --------------------------------------------------------------------------
// I have two old Pentium-4 based desktop machines that I use for development.
// One is a Dell Dimension 2400 and the other is a custom built and based upon
// a Micro-Star Int'l MS-7082 motherboard. Each of these machines has a ethernet
// controller chip soldered to the motherboard. The Dell machine has a Broadcom
// chip and the other machine has a RealTek RTL8100C. Each machine also has a 
// generic PCI ethernet adapter card based upon the RealTek RTL8139D. 
// Naturally, I want to be able to use eth0 during development. So, I want the 
// device driver '8139too' to be bound to the rtl8100C chip on the motherboard. 
// Obviously, I also want my driver to work for the RTL8139D on the PCI card. 
// Luckily, the two devices can be differentiated by the subsystem vendor id. 
// The rtl8100c soldered to the motherboard has subsystem vendor ID == 0x1462 
// (Micro-Star Int'l). In order to achieve these twin goals, I've hacked up 
// the sources for the 8139too driver (on the other machine) so that it only 
// supports an 8139 family device with the subsystem vendor id, 0x1462. 
// My driver will only support the generic Realtek Subsystem VendorID.
static struct pci_device_id bks_pci_device_ids[] __devinitdata = {
    {0x10ec, 0x8139, 0x10ec, 0x8139, 0, 0, 0},
    { 0, 0}
};

MODULE_DEVICE_TABLE(pci, bks_pci_device_ids);



// --------------------------------------------------------------------------
// Receive buffer is 32KB but, in some instances, the device can write past
// the advertised end of buffer when we say it is OK to do so.
#define RX_BUF_SIZE  (4094 << 3)
#define RX_BUF_STATUS_SIZE (16)
#define RX_BUF_NOWRAP_SIZE (2048)
#define RX_BUF_TOT_SIZE  (RX_BUF_SIZE + RX_BUF_STATUS_SIZE + RX_BUF_NOWRAP_SIZE)

#define TX_DESCR_CNT (4)
#define MAX_ETH_FRAME_SIZE  (1536)
#define TX_BUF_SIZE MAX_ETH_FRAME_SIZE
#define TX_BUF_TOT_SIZE (TX_BUF_SIZE * TX_DESCR_CNT )

// use port I/O instead of memory mapped I/O in order to (hopefully)
// avoid the so called "write post bug". 
#define RTL8139BKS_USE_PIO



// --------------------------------------------------------------------------
// define private data structure
struct bks_private {
    struct pci_dev* pci_dev;
    struct net_device* net_dev;

    unsigned long pio_base; // base address of port mapped io registers. 
    void* __iomem ioaddr; // base address of the memory mapped io registers

    u16 intr_mask; // the interrupts of interest.

    unsigned int tx_front; // index into the tx_buf array. remove from front
    unsigned int tx_back; // index into the tx_buf array. push back
    unsigned char* tx_buf[TX_DESCR_CNT]; // an array of pointers. each one points to the start of a Tx Buffer.
    unsigned char* tx_bufs; // points to a contiguous region large enough for four Tx buffers.
    dma_addr_t tx_bufs_dma;

    void* rx_ring;
    unsigned int cur_rx;
    dma_addr_t rx_ring_dma;
    unsigned int rx_config;

    struct net_device_stats stats;

    // spinlock protects access to chip's registers?
    // or, protects access to the fields in this structure? 
    spinlock_t lock;
};

// --------------------------------------------------------------------------
// forward declarations of functions
// --------------------------------------------------------------------------
static unsigned int bks_rd8(struct bks_private*, int offset);
static unsigned int bks_rd16(struct bks_private*, int offset);
static unsigned int bks_rd32(struct bks_private*, int offset);
static int bks_wr8(struct bks_private*, int offset, unsigned int val);
static int bks_wr16(struct bks_private*, int offset, unsigned int val);
static int bks_wr32(struct bks_private*, int offset, unsigned int val);

static int bks_reset_chip (struct bks_private*);
static int bks_init_hardware(struct bks_private*);
static int bks_stop_hardware(struct bks_private*);
static int is_tx_queue_full(struct bks_private*);
static int bks_handle_tx_intr(struct bks_private*);

static int bks_ndo_open(struct net_device* net_dev);
static int bks_ndo_close(struct net_device* net_dev);
static int bks_ndo_hard_start_xmit(struct sk_buff* skb, struct net_device* net_dev);
static struct net_device_stats* bks_ndo_get_stats(struct net_device* net_dev);

static int __devinit bks_pci_probe(struct pci_dev* pci_dev, const struct pci_device_id* id);
static void __devexit bks_pci_remove(struct pci_dev* pci_dev);
static irqreturn_t bks_interrupt(int irq, void* dev_id);

static int __init bks_mod_init(void);
static void __exit bks_mod_exit(void);

// --------------------------------------------------------------------------
// globals.
// --------------------------------------------------------------------------
static struct pci_driver bks_driver = {
    .name = DRV_NAME,
    .id_table = bks_pci_device_ids,
    .probe = bks_pci_probe,
    .remove = __devexit_p(bks_pci_remove),
    //.suspend  = bks_pci_suspend,
    //.resume   = bks_pci_resume,
};

// --------------------------------------------------------------------------
#ifdef HAVE_NET_DEVICE_OPS
static struct net_device_ops bks_netdev_ops = {
    .ndo_open = bks_ndo_open,
    .ndo_stop = bks_ndo_close,
    .ndo_get_stats = bks_ndo_get_stats,
    .ndo_start_xmit = bks_ndo_hard_start_xmit
};
#endif

// --------------------------------------------------------------------------
// ************************ private functions ****************************
// --------------------------------------------------------------------------
//
unsigned int bks_rd8(struct bks_private* priv, int offset)
{
    unsigned int regval = 0;
#ifdef RTL8139BKS_USE_PIO
    regval = inb(priv->pio_base + offset);
#else
    regval = ioread8(priv->ioaddr + offset);
#endif
    return regval;
}

// --------------------------------------------------------------------------
unsigned int bks_rd16(struct bks_private* priv, int offset)
{
    unsigned int regval = 0;
#ifdef RTL8139BKS_USE_PIO
    regval = inw(priv->pio_base + offset);
#else
    regval = ioread16(priv->ioaddr + offset);
#endif
    return regval;
}

// --------------------------------------------------------------------------
unsigned int bks_rd32(struct bks_private* priv, int offset)
{
    unsigned int regval = 0;
#ifdef RTL8139BKS_USE_PIO
    regval = inl(priv->pio_base + offset);
#else
    regval = ioread32(priv->ioaddr + offset);
#endif
    return regval;
}

// --------------------------------------------------------------------------
int bks_wr8(struct bks_private* priv, int offset, unsigned int val)
{
    int rc = 0;
#ifdef RTL8139BKS_USE_PIO
    outb(val, priv->pio_base + offset);
#else
    iowrite8(val, priv->ioaddr + offset);
#endif
    return rc;
}

// --------------------------------------------------------------------------
int bks_wr16(struct bks_private* priv, int offset, unsigned int val)
{
    int rc = 0;
#ifdef RTL8139BKS_USE_PIO
    outw(val, priv->pio_base + offset);
#else
    iowrite16(val, priv->ioaddr + offset);
#endif
    return rc;
}

// --------------------------------------------------------------------------
int bks_wr32(struct bks_private* priv, int offset, unsigned int val)
{
    int rc = 0;
#ifdef RTL8139BKS_USE_PIO
    outl(val, priv->pio_base + offset);
#else
    iowrite32(val, priv->ioaddr + offset);
#endif
    return rc;
}

// --------------------------------------------------------------------------
int bks_reset_chip (struct bks_private* priv)
{
    int rc = 1;
   //void* __iomem iobase = priv->ioaddr;

    // command a soft reset
    bks_wr8(priv, ChipCmd, CmdSoftReset);

    // wait here until the chip says it has completed the reset
    for (int i = 1000; 0 < i; --i) {
        barrier();
        if (0 == (bks_rd8(priv, ChipCmd) & CmdSoftReset)) {
            rc = 0; // success.
            break;
        }
        udelay(10);
    }
    return rc;
}



// --------------------------------------------------------------------------
static int bks_init_hardware(struct bks_private* priv)
{
    int rc = 0;
    unsigned int regval = 0;
    unsigned int reserved;
    unsigned int hw_version_id = 0;

    void* __iomem iobase = priv->ioaddr;

    printk(KERN_INFO "%s init_hardware()\n", DRV_NAME);

    if (0 != bks_reset_chip(priv)) {
        printk(KERN_WARNING "%s timedout waiting for reset to complete\n", DRV_NAME);
    }

    // read and report the media status
    regval = ioread8(iobase + MediaStatus);
    printk(KERN_INFO "%s Media Status: 0x%02x\n", DRV_NAME, regval);
    //printk(KERN_INFO "%s \n", DRV_NAME);

    // tell the device the bus address of our receive buffer. 
    bks_wr32(priv, RxBufStartAddr, priv->rx_ring_dma);

    // command the chip to enable receive and transmit.
    regval = bks_rd8(priv, ChipCmd);
    regval = regval | CmdTxEnable | CmdRxEnable;
    bks_wr8(priv, ChipCmd, regval);

    // set the receive config
    regval = bks_rd32(priv, RxConfig);
    reserved = regval & 0xf0fc0040; // these bits are marked reserved on the data sheet.
    regval = regval | priv->rx_config;
    bks_wr32(priv, RxConfig, regval);
    regval = bks_rd32(priv, RxConfig);
    printk(KERN_INFO "%s Rx Config: 0x%08x\n", DRV_NAME, regval);

    // discover the HW version Id
    regval = bks_rd32(priv, TxConfig);
    hw_version_id = ((regval & 0x7c000000)>>24) | ((regval & 0x00c00000)>>22);
    printk(KERN_INFO "%s HW Version Id: 0x%02x\n", DRV_NAME, hw_version_id);
    // and set the Tx config.
    regval = regval | (3<<24) | (3<<9);
    regval = regval & ~((3<<17) | (1<<8) | (0x0f<<4)); // clear bits 4,5,6,7,8,17 and 18.
    bks_wr32(priv, TxConfig, regval);
    regval = bks_rd32(priv, TxConfig);
    printk(KERN_INFO "%s Tx Config: 0x%08x\n", DRV_NAME, regval);


    // set the physical address of each of the TX buffers
    for (int i=0; i<TX_DESCR_CNT; ++i) {
        unsigned int offset;
        offset = priv->tx_buf[i] - priv->tx_bufs; // offset of this tx buf relative to start 
        bks_wr32(priv, TxAddr0 + 4*i, priv->tx_bufs_dma + offset);
    }


    bks_wr32(priv, RxMissedCount, 0); // reset the missed packet counter.
    bks_wr32(priv, TimerCount, 0); // reset the timer/counter
    bks_wr32(priv, TimerInterval, 0x7fffffff); // ask for a interrupt after some time has passed.
    barrier();

    // disable multi-interrrupts ?
    regval = bks_rd16(priv, MultiIntr);
    regval &= 0x0f000; // clear all non-reserved bits
    bks_wr16(priv, MultiIntr, regval);

    // clear any pending interrupt.
    regval = bks_rd16(priv, IntrStatus);
    regval = regval | ~0x1f80; // bits [12..7] are reserved. Set all the reset.
    bks_wr16(priv, IntrStatus, regval);

    // enable some interrupts
    regval = priv->intr_mask;
    regval |= TimerTimedOut; // enable the counter/timer interrupt too.
    bks_wr16(priv, IntrMask, regval);
    barrier();

    return rc;
}


// --------------------------------------------------------------------------
// undo what was done in init_hardware() 
int bks_stop_hardware(struct bks_private* priv)
{
    int rc = 0;
    //void* __iomem iobase = priv->ioaddr;
    unsigned int regval = 0;

    // disable all interrupts 
    bks_wr16(priv, IntrMask, 0);
    
    // command chip to disable receive and transmit
    regval = bks_rd8(priv, ChipCmd);
    regval = regval & ~(CmdTxEnable | CmdRxEnable);
    bks_wr8(priv, ChipCmd, regval);

    return rc;
}

// --------------------------------------------------------------------------
// this is not quite correct
int how_many_bufs_in_tx_queue(struct bks_private* priv)
{
    int n = TX_DESCR_CNT;
    int count = 0;
    count = (n + priv->tx_back - priv->tx_front) % n;
    return count;
}

// --------------------------------------------------------------------------
int is_tx_queue_full(struct bks_private* priv)
{
   return !(how_many_bufs_in_tx_queue(priv) < TX_DESCR_CNT - 1);
}

// --------------------------------------------------------------------------
// purpose is mainly to clean up the tx buffers and fix up the accounting. 
// it is assumed that the caller holds the spinlock when this fctn is called. 
int bks_handle_tx_intr(struct bks_private* priv)
{
    int handled = 1;
    struct net_device* net_dev = priv->net_dev;
    unsigned int idx; // an index into the tx_buf array
    unsigned int txstatus = 0;
    int count = 0;

    idx = priv->tx_front % TX_DESCR_CNT;
    while (0 < how_many_bufs_in_tx_queue(priv)) {
        txstatus = bks_rd32(priv, TxStatus0 + (idx * sizeof(u32)));

        if (/*!(txstatus & (TxStatusTxComplete)) ||*/ !(txstatus & TxStatusOwn)) {
            // he's not done with this buffer yet.
            break;
        }

        if (txstatus & TxStatusTxOk) {
            priv->stats.tx_bytes += (txstatus & TxStatusTxByteCountMask);
            priv->stats.tx_packets += 1;
        }

        idx = (idx + 1) % TX_DESCR_CNT;
        priv->tx_front = idx;
        ++count;
    } // while

    mb(); // why? 

    if (count && netif_queue_stopped(net_dev)) {
        printk(KERN_INFO "%s wake TX queue\n", DRV_NAME);
        netif_wake_queue(net_dev);
    }

    return handled;
}





// --------------------------------------------------------------------------
// **************** net device ops ****************************** 
// --------------------------------------------------------------------------
int bks_ndo_open(struct net_device* net_dev)
{
    int rc = 0;
    struct bks_private* priv = netdev_priv(net_dev);
    //void* __iomem ioaddr = priv->ioaddr;

    printk(KERN_INFO "%s ndo_open()\n", DRV_NAME);

    // register our interrupt handler.
    rc = request_irq(net_dev->irq, bks_interrupt, IRQF_SHARED, net_dev->name, net_dev);
    if (rc != 0) {
        goto exit; // bail out with this error code.
    }

    // allocate DMA buffers for both receive and transmit
    // void* pci_alloc_consistent(struct pci_dev*, size_t, dma_add_t*);
    // is a convenient wrapper around...
    // void* dma_alloc_coherent(struct device*, size_t, dma_addr_t*, gfp_t);
    priv->tx_bufs = pci_alloc_consistent(priv->pci_dev, TX_BUF_TOT_SIZE, &priv->tx_bufs_dma);
    if (!priv->tx_bufs) {
        rc = -ENOMEM;
        goto err_out_free_irq;
    }
    priv->rx_ring = pci_alloc_consistent(priv->pci_dev, RX_BUF_TOT_SIZE, &priv->rx_ring_dma);
    if (!priv->rx_ring) {
        rc = -ENOMEM;
        goto err_out_free_dma;
    }


    // nowrap, accept packets that match my hw address, accept broadcast, 
    // no rx thresh, rx buf len = 32k+16max rx dma burst 1024, 
    priv->rx_config = 0x0000f68a; // todo: create nice symbolic names for these bits. 

    //priv->intr_mask = RxOK|RxError|TxOK|TxError|RxBufferOverflow|RxFifoOverflow|SystemError|PacketUnderRunLinkChange|TimerTimedOut;
    priv->intr_mask = RxOK|RxError|TxOK|TxError|RxBufferOverflow|RxFifoOverflow|SystemError|PacketUnderRunLinkChange;

    // initialize all of the rest of the private data
    // especially the buffer accounting.
    priv->cur_rx = 0;
    priv->tx_back = 0;
    priv->tx_front = 0;
    for (int i=0; i<TX_DESCR_CNT; ++i) {
        priv->tx_buf[i] = &priv->tx_bufs[i * TX_BUF_SIZE];
    }

    bks_init_hardware(priv);

    netif_start_queue(net_dev);
    rc = 0; // if we get this far all is well.
    goto exit; // successfully.

err_out_free_dma:
    if (priv->rx_ring) {
        pci_free_consistent(priv->pci_dev, RX_BUF_TOT_SIZE, priv->rx_ring, priv->rx_ring_dma);
    }
    if (priv->tx_bufs) {
        pci_free_consistent(priv->pci_dev, TX_BUF_TOT_SIZE, priv->tx_bufs, priv->tx_bufs_dma);
    }

err_out_free_irq:
    free_irq(net_dev->irq, net_dev);

exit:
    return rc;
}

// --------------------------------------------------------------------------
int bks_ndo_close(struct net_device* net_dev)
{
    int rc = 0;
    struct bks_private* priv = netdev_priv(net_dev);
    //void* __iomem iobase = priv->ioaddr;

    printk(KERN_INFO "%s ndo_close()\n", DRV_NAME);

    // stop transmission
    netif_stop_queue(net_dev);

    bks_stop_hardware(priv);

    // update statistics

    // free dma buffers
    if (priv->rx_ring) {
        pci_free_consistent(priv->pci_dev, RX_BUF_TOT_SIZE, priv->rx_ring, priv->rx_ring_dma);
    }

    if (priv->tx_bufs) {
        pci_free_consistent(priv->pci_dev, TX_BUF_TOT_SIZE, priv->tx_bufs, priv->tx_bufs_dma);
    }

    // free the irq
    free_irq(net_dev->irq, net_dev);


    // re-set transmit accounting.
    priv->tx_back = 0;
    priv->tx_front = 0;


    return rc;
}


// --------------------------------------------------------------------------
// copy the data from the skb to a tx dma buffer and tell the chip to 
// start DMA and transmission. then, free the skb
int bks_ndo_hard_start_xmit(struct sk_buff* skb, struct net_device* dev)
{
    int rc = NETDEV_TX_OK;
    struct bks_private* priv = netdev_priv(dev);
    unsigned int idx; // an index into the tx_buf array
    unsigned int len = skb->len;
    unsigned long flags;
    unsigned int reg = TxStatus0;
    unsigned int val = 0;
    

    printk(KERN_INFO "%s ndo_hard_start_xmit()\n", DRV_NAME);

    if (unlikely(TX_BUF_SIZE < len)) {
        dev_kfree_skb(skb);
        priv->stats.tx_dropped += 1;
        goto exit;
    }


    idx = (priv->tx_back) % TX_DESCR_CNT;

    if (len < ETH_ZLEN) {
        memset(priv->tx_buf[idx], 0, ETH_ZLEN);
    }

    skb_copy_and_csum_dev(skb, priv->tx_buf[idx]);
    dev_kfree_skb(skb);

    spin_lock_irqsave(&priv->lock, flags);

    wmb();
    reg = TxStatus0 + (idx * sizeof(u32));
    // early Tx threshold: 32*32 = 1024 bytes
    val = ((32<<TxStatusEarlyTxThreshShift) & TxStatusEarlyTxThreshMask);
    // TX data size: len.
    val |= (len & TxStatusTxByteCountMask);
    // tell the device that it owns the buffer now.
    val &= ~TxStatusOwn;

    bks_wr32(priv, reg, val);
    val = bks_rd32(priv, reg);
    printk(KERN_INFO "%s, wrote 0x%08x to register 0x%08x\n", DRV_NAME, val, reg);

    priv->tx_back = (idx + 1) % TX_DESCR_CNT;
    if (is_tx_queue_full(priv)) {
        printk(KERN_INFO "%s pause TX queue.\n", DRV_NAME);
        netif_stop_queue(dev);
    }

    spin_unlock_irqrestore(&priv->lock, flags);

exit: 
    return rc;
}

// --------------------------------------------------------------------------
struct net_device_stats* bks_ndo_get_stats(struct net_device* dev)
{
    struct net_device_stats* stats = 0;
    struct bks_private* priv = 0;

    //printk(KERN_INFO "%s ndo_get_stats()\n", DRV_NAME);
    
    priv = netdev_priv(dev);
    stats = &priv->stats;

    return stats;
}


// --------------------------------------------------------------------------
// **************** PCI device ****************************** 
// --------------------------------------------------------------------------
int __devinit bks_pci_probe(struct pci_dev* pci_dev, const struct pci_device_id* id)
{
    int rc = 0;
    struct net_device* net_dev;
    struct bks_private* priv = 0;
    unsigned long pio_start, pio_end, pio_len, pio_flags;
    unsigned long mmio_start, mmio_end, mmio_len, mmio_flags;
    void* __iomem ioaddr;

    printk(KERN_INFO "%s pci_probe()\n", DRV_NAME);
    
    // allocate an ethernet device.
    // this also allocates space for the private data and initializes it to zeros. 
    net_dev = alloc_etherdev(sizeof(struct bks_private));
    if (!net_dev) {
        rc = -ENOMEM;
        goto out;
    }
    SET_NETDEV_DEV(net_dev, &pci_dev->dev);
    // initialize some fields in the net device. 
    memcpy(&net_dev->name[0], DEV_NAME, sizeof DEV_NAME);
    net_dev->irq = pci_dev->irq;
    net_dev->hard_header_len = 14;

    // initialize some fields in the private data
    priv = netdev_priv(net_dev);
    priv->pci_dev = pci_dev;
    priv->net_dev = net_dev;
    spin_lock_init(&priv->lock);

    // enable the pci device
    rc = pci_enable_device(pci_dev);
    if (rc) {
        goto err_out_free_netdev;
    }

    // keep a hook to the private stuff in the pci_device too. 
    pci_set_drvdata(pci_dev, priv);

    // enable bus master
    pci_set_master(pci_dev);

    // find out where the io regions are.
    pio_start = pci_resource_start (pci_dev, 0);
    pio_end = pci_resource_end (pci_dev, 0);
    pio_len = pci_resource_len (pci_dev, 0);
    pio_flags = pci_resource_flags (pci_dev, 0);
    printk(KERN_INFO "%s io region 0 start: 0x%08lx, end: 0x%08lx, length: 0x%lx, flags: 0x%08lx\n", 
                      DRV_NAME, pio_start, pio_end, pio_len, pio_flags);

    mmio_start = pci_resource_start (pci_dev, 1);
    mmio_end = pci_resource_end (pci_dev, 1);
    mmio_len = pci_resource_len (pci_dev, 1);
    mmio_flags = pci_resource_flags (pci_dev, 1);
    printk(KERN_INFO "%s io region 1 start: 0x%08lx, end: 0x%08lx, length: 0x%lx, flags: 0x%08lx\n", 
                      DRV_NAME, mmio_start, mmio_end, mmio_len, mmio_flags);


    // make sure that region #1 is memory mapped.
    if (!(mmio_flags & IORESOURCE_MEM)) {
        printk(KERN_WARNING "%s IO resource region #1 is not memory mapped - aborting now.\n", DRV_NAME);
        rc = -ENODEV;
        goto err_out_clear_master;
    }


    // take ownership of all the io regions (in this case there are only two)
    rc = pci_request_regions(pci_dev, DRV_NAME);
    if (rc) {
        goto err_out_clear_master;
    }

    // ask kernel to setup page tables for the mmio region.
    ioaddr = pci_iomap(pci_dev, 1, 0);
    if (!ioaddr) {
        rc = -EIO;
        goto err_out_release_regions;
    }

    // save off the base address of the io registers
    net_dev->base_addr = (long)ioaddr;
    priv->ioaddr = ioaddr;
    priv->pio_base = pio_start;

    // configure 32bit DMA.
    rc = pci_set_dma_mask(pci_dev, DMA_BIT_MASK(32));
    if (rc) {
        goto err_out_iounmap;
    }

//  rc = pci_set_consistent_dma_mask(pci_dev, DMA_BIT_MASK(32));
//  if (rc) {
//      goto err_out_iounmap;
//  }

    memset(&net_dev->broadcast[0], 255, 6);
    // copy the six byte ethernet address from the io memory to the net_device structure.
    memcpy_fromio(&net_dev->dev_addr[0], (ioaddr+EthernetAddr), 6);

#ifdef HAVE_NET_DEVICE_OPS
    net_dev->netdev_ops = &bks_netdev_ops;
#else
    net_dev->open = bks_ndo_open;
    net_dev->stop = bks_ndo_close;
    net_dev->hard_start_xmit = bks_ndo_hard_start_xmit;
    net_dev->get_stats = bks_ndo_get_stats;
#endif

    rc = register_netdev(net_dev);
    if (rc) {
        goto err_out_iounmap;
    }

    rc = 0;
    goto out;

err_out_iounmap:
    pci_iounmap(pci_dev, ioaddr);

err_out_release_regions:
    pci_release_regions(pci_dev);

//err_out_mwi:
//  pci_clear_mwi(pci_dev);

err_out_clear_master:
    pci_clear_master(pci_dev);

//err_out_disable:
    pci_disable_device(pci_dev);

err_out_free_netdev:
    free_netdev(net_dev);

out:
    return rc;
}

// --------------------------------------------------------------------------
void __devexit bks_pci_remove(struct pci_dev* pci_dev)
{
    struct net_device* net_dev = 0;
    struct bks_private* priv = 0;
    void* __iomem ioaddr = 0;

    printk(KERN_INFO "%s pci_remove()\n", DRV_NAME);

    priv = (struct bks_private*)pci_get_drvdata(pci_dev);
    if (priv) {
        net_dev = priv->net_dev;
        ioaddr = priv->ioaddr;

        if (net_dev) {
            // this will cause bks_ndo_close() to be called. 
            unregister_netdev(net_dev);
        }

        // disable interrupts
        //bks_wr16(priv, IntrMask, 0);
        //bks_rd16(priv, IntrMask);
        
        pci_iounmap(pci_dev, ioaddr);
        priv->ioaddr = 0; // prophy.
    }

    pci_release_regions(pci_dev);
    pci_clear_master(pci_dev);
    pci_set_drvdata(pci_dev,0);
    pci_disable_device(pci_dev);

    if (net_dev) {
        free_netdev(net_dev);
    }

}

// --------------------------------------------------------------------------
// the interrupt service function. 
irqreturn_t bks_interrupt(int irq, void* dev_id)
{
    int handled = 0;
    struct net_device* net_dev = (struct net_device*)dev_id;
    struct bks_private* priv = netdev_priv(net_dev);
    unsigned int intr_stat = 0;
    unsigned int ack = 0;
    //void* __iomem iobase = priv->ioaddr;

    // this is way too chatty....
    //printk(KERN_INFO "%s interrupt(), irq: %d, dev_id: %p, status: 0x%04x\n", DRV_NAME, irq, dev_id, intr_stat);

    // read the interrupt status register
    intr_stat = bks_rd16(priv, IntrStatus);

    // spurious interrrupt?
    if ((0xffff == intr_stat) || (0 == intr_stat)) {
        handled = 0;
        goto exit;
    }
    
    printk(KERN_INFO "%s intr status: 0x%04x\n", DRV_NAME, intr_stat);

    // grab the lock and hold it for the entire duration of this fctn.
    spin_lock(&priv->lock);

    // counter/timer interrupt? 
    if (intr_stat & TimerTimedOut) {
        // the timer timed out
        ack |= TimerTimedOut;
        bks_wr16(priv, IntrMask, priv->intr_mask); // disable the timer interrupt!
        bks_wr32(priv, TimerCount, 0); // reset the counter
        barrier();
        printk(KERN_INFO "%s counter/timer interrupt.\n", DRV_NAME);
        handled = 1;
    }
    
    // receive interrupt? 
    if (intr_stat & (RxOK | RxBufferOverflow | RxFifoOverflow)) {
        // deal with received packet.
        ack |= (intr_stat & (RxOK | RxBufferOverflow | RxFifoOverflow));
        handled = 1;
    }

    // transmit complete interrupt?
    if (intr_stat & (TxOK | TxError)) {
        // deal with transmission.
        ack |= (intr_stat & (TxOK | TxError));
        handled = bks_handle_tx_intr(priv);
    }

    // ack all the interrupts that were serviced
    bks_wr16(priv, IntrStatus, ack);

    spin_unlock(&priv->lock);

exit:
    return IRQ_RETVAL(handled);
}



// --------------------------------------------------------------------------
// ************************** module ******************************
// --------------------------------------------------------------------------
int __init bks_mod_init(void)
{
    int rc = 0;
    printk(KERN_INFO "%s mod_init()\n", DRV_NAME);
    rc = pci_register_driver(&bks_driver);
    return rc;
}

// --------------------------------------------------------------------------
void __exit bks_mod_exit(void)
{
    printk(KERN_INFO "%s mod_exit()\n", DRV_NAME);
    pci_unregister_driver(&bks_driver);
}

module_init(bks_mod_init);
module_exit(bks_mod_exit);

