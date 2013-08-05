//#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

//#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs.h> // struct block_device and much more.
#include <linux/errno.h>
//#include <linux/timer.h>
#include <linux/types.h> 
#include <linux/fcntl.h>  // O_ACCMODE
#include <linux/hdreg.h>  // HDIO_GETGEO
//#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
//#include <linux/buffer_head.h>
//#include <linux/bio.h>

// -------------------------------------------------------------------------
// adapted from sbull.c in the examples accompanying the book,
// "Linux Device Drivers - 3rd Edition" 
// by J. Corbet, A Rubini and G. Kroah-Hartman.
// actually, significant changes were made in the block device kernel api
// after that example code was published and so, much of this has departed
// from the example code. The structure however, is pretty much the same.
// then I found this...
// http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/
// which helped alot. 
// but it still does not really work properly?
// hangs when I try to do fdisk /dev/bksblk

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");

static int bks_major = 0;
module_param(bks_major, int, 0644);

static int cylinders = 64;
module_param(cylinders, int, 0644);

static int heads = 2;
module_param(heads, int, 0644);

static int sectors_per_track = 16;
module_param(sectors_per_track, int, 0644);


// -------------------------------------------------------------------------
struct BksDeviceContext {
	int size; // in bytes.
	unsigned char* data;
	spinlock_t lock;
	struct request_queue *queue;
	struct gendisk *gd;
};


// -------------------------------------------------------------------------
// an array of device context structures. 
static struct BksDeviceContext* BksDeviceCtx = 0;

// -------------------------------------------------------------------------
static void bks_transfer(struct BksDeviceContext* ctx, unsigned long sector, unsigned long sector_count, char* buffer, int write)
{
	unsigned long offset = sector*512;
	unsigned long byte_count = sector_count*512;

	printk(KERN_INFO "[BKSBLK] transfer(), ctx: %p, sector: %lu, count: %lu, buffer: %p, write: %d\n", ctx, sector, sector_count, buffer, write);
	//printk(KERN_INFO "[BKSBLK] transfer(), offset: %ld,  byte count %ld)\n", offset, byte_count);

	if (!ctx || !buffer) {
		; // do nothing.
	} else if (ctx->size < (offset + byte_count)) {
		printk (KERN_WARNING "[BKSBLK] transfer(), size (%d) is smaller than  offset (%ld)  plus  byte count (%ld)\n", ctx->size, offset, byte_count);
	} else if (write) {
		memcpy(&ctx->data[offset], buffer, byte_count);
	} else {
		memcpy(buffer, &ctx->data[offset], byte_count);
	}
}


// -------------------------------------------------------------------------
// this isn't quite right but, it doesn't crash the system or hang! 
// can do fdisk or mke2fs 
static void bks_request(struct request_queue* rq)
{
	struct request* req = 0;
	struct BksDeviceContext* ctx = 0;
	unsigned long sector = 0;
	unsigned long count = 0;
	char* buffer = 0;
	int write = 0;

	if (rq) { 
		ctx = rq->queuedata;
	}

	req = blk_fetch_request(rq);
	printk(KERN_INFO "[BKSBLK] request(), request_queue: %p, ctx: %p, req: %p\n", rq, ctx, req);

	if (req && (req->cmd_type == REQ_TYPE_FS)) {
		sector = blk_rq_pos(req); 
		count = blk_rq_cur_sectors(req);
		buffer = req->buffer;
		write = rq_data_dir(req);
		bks_transfer(ctx, sector, count, buffer, write);
		__blk_end_request_all(req, 0);
	} else {
		__blk_end_request_all(req, -EIO);
	}
}



// -------------------------------------------------------------------------
static int bks_open(struct block_device* bd, fmode_t fmode)
{
	//struct BksDeviceContext* ctx = (struct BksDeviceCtx*)bd->bd_private; // must claim bd???

	printk(KERN_INFO "[BKSBLK] open()\n");

	//spin_lock(&ctx->lock);
	//if (0 == ctx->users) {
	//	check_disk_change(inode->i_bdev);
	//}
	//ctx->users++;
	//spin_unlock(&ctx->lock);

	return 0;
}

// -------------------------------------------------------------------------
static int bks_release(struct gendisk* gd, fmode_t fmode)
{
	printk(KERN_INFO "[BKSBLK] release()\n");
	//struct BksDeviceContext* ctx = ???

	//spin_lock(&ctx->lock);
	//ctx->users--;
	//spin_unlock(&ctx->lock);

	return 0;
}


// -------------------------------------------------------------------------
int bks_getgeo(struct block_device* bd, struct hd_geometry* geo)
{
	int rc = 0;
	printk(KERN_INFO "[BKSBLK] getgeo()\n");

	if (geo) {
		geo->cylinders = cylinders;
		geo->heads = heads;
		geo->sectors = sectors_per_track;
		geo->start = 4;
	} else {
		rc = -EINVAL;
	}

	return rc;
}



// -------------------------------------------------------------------------
static struct block_device_operations bks_ops = {
	.owner = THIS_MODULE,
	.open = bks_open,
	.release = bks_release,
	.getgeo = bks_getgeo,
};


// -------------------------------------------------------------------------
static int initialize_device(struct BksDeviceContext* ctx, int device_number)
{
	int rc = 0;
	int total_sectors = cylinders * heads * sectors_per_track;

	printk(KERN_INFO "[BKSBLK] initialize_device() - enter\n");

	memset (ctx, 0, sizeof(struct BksDeviceContext));

	ctx->size = total_sectors * 512; // total bytes.
	ctx->data = vmalloc(ctx->size);
	if (!ctx->data) {
		printk (KERN_WARNING "vmalloc failed.\n");
		rc = -ENOMEM;
		goto exit;
	}

	memset(ctx->data, 0 ,ctx->size);

	spin_lock_init(&ctx->lock);

	ctx->queue = blk_init_queue(bks_request, &ctx->lock);
	if (!ctx->queue) {
		printk(KERN_WARNING "[BKSBLK] initialize_device(), blk_init_queue() failed\n");
		vfree(ctx->data);
		ctx->data =0;
		rc = -ENOMEM;
		goto exit;
	}

	//blk_queue_hardsect_size(ctx->queue, 512);
	ctx->queue->queuedata = ctx;

	ctx->gd = alloc_disk(1);// magic: number of minors but, not sure how this works?
	if (!ctx->gd) {
		printk(KERN_WARNING "[BKSBLK] initialize_device(), alloc_disk() failed\n");
		vfree(ctx->data);
		ctx->data =0;
		rc = -ENOMEM;
		goto exit;
	}

	ctx->gd->major = bks_major;
	ctx->gd->first_minor = 0;
	ctx->gd->fops = &bks_ops;
	ctx->gd->queue = ctx->queue;
	ctx->gd->private_data = ctx;
	snprintf (ctx->gd->disk_name, 32, "bksblk");
	set_capacity(ctx->gd, total_sectors);
	add_disk(ctx->gd);

	rc = 0;


exit:
	printk(KERN_INFO "[BKSBLK] initialize_device() - exit\n");

	return rc;
}



// -------------------------------------------------------------------------
static int __init bks_init(void)
{
	int rc = 0;

	bks_major = register_blkdev(bks_major, "bksblk");
	printk(KERN_INFO "[BKSBLK] init(), register_blkdev() returned major number: %d\n", bks_major);

	if (bks_major <= 0) {
		rc = -EBUSY;
	} else if (0 == (BksDeviceCtx = kmalloc(sizeof(struct BksDeviceContext), GFP_KERNEL))) {
		unregister_blkdev(bks_major, "bksblk");
		rc = -ENOMEM;
	} else {
		int i = 0;
		//for (i=0; i<device_count; ++i) {
			rc = initialize_device(BksDeviceCtx, i);
		//}
	}

	return rc;
}

// -------------------------------------------------------------------------
static void __exit bks_exit(void)
{
	struct BksDeviceContext* ctx = BksDeviceCtx;

	printk(KERN_INFO "[BKSBLK] exit()\n");

	if (ctx) {
		if (ctx->gd) {
			del_gendisk(ctx->gd);
			put_disk(ctx->gd);
		}

		if (bks_major) {
			unregister_blkdev(bks_major, "bksblk");
		}

		if (ctx->queue) {
			blk_cleanup_queue(ctx->queue);
		}

		if (ctx->data) {
			vfree(ctx->data);
		}
	
		if (BksDeviceCtx) { 
			kfree(BksDeviceCtx);
		}
	}
}

module_init(bks_init);
module_exit(bks_exit);

