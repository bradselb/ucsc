#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gfp.h>  // get_zeroed_page(), free_page()
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>  // strncpy(), strnlen()
#include <asm/uaccess.h> // copy to/from user

#include "bks_ringbuf.h"


// -------------------------------------------------------------------------
// my cache of BksRingBuffer objects.
static struct kmem_cache* bks_ringbuf_cache;


// -------------------------------------------------------------------------
struct BksRingBuffer
{
	unsigned int* base;
	unsigned int rd_idx;
	unsigned int wr_idx;
	unsigned int size;
	spinlock_t lock; // protect the read and write indeces. 
};

// -------------------------------------------------------------------------
int bks_ringbuf_init()
{
	int rc = 0;
	struct kmem_cache* cache = 0;
	cache = kmem_cache_create("BksRingBuffer", sizeof(struct BksRingBuffer), 0, SLAB_POISON, 0);
	if (cache) {
		bks_ringbuf_cache = cache;
		rc = 0;
	} else {
		rc = -ENOMEM;
	}
	return rc;

}

// -------------------------------------------------------------------------
void bks_ringbuf_cleanup()
{
	if (bks_ringbuf_cache) {
		kmem_cache_destroy(bks_ringbuf_cache);
		bks_ringbuf_cache = 0;
	}
}



// -------------------------------------------------------------------------
// allocate a BksRingBuffer object from the cache and allocate 
// the buffer using get_zeroed_page()
struct BksRingBuffer* bks_ringbuf_allocate()
{
	struct BksRingBuffer* buf = 0;
	if (!bks_ringbuf_cache) {
		; // fail - return 0
	} else if (0 == (buf = kmem_cache_alloc(bks_ringbuf_cache, GFP_KERNEL))) {
		; // fail - return 0
	} else if (0 == (buf->base = (unsigned int*)get_zeroed_page(GFP_KERNEL))) {
		// fail but, need to cleanup a bit. 
		kmem_cache_free(bks_ringbuf_cache, buf);
		buf = 0;
	} else {
		// success! finish the job
		buf->size = PAGE_SIZE / sizeof(unsigned int);
		buf->rd_idx = 0;
		buf->wr_idx = 0;
		spin_lock_init(&buf->lock);
	}

	return buf;
}

// -------------------------------------------------------------------------
void bks_ringbuf_free(struct BksRingBuffer* buf)
{
	printk(KERN_INFO "[bks] ringbuf_free(), buf: %p, rd_idx: %u, wr_idx: %u\n",
	     buf, buf->rd_idx, buf->wr_idx);
	if (buf) {
		if (buf->base) {
			memset(buf->base, 0, buf->size * sizeof(*buf->base));
			free_page((unsigned long)buf->base);
			buf->base = 0;
			// no need to clean up the spin locks? 
		}
		kmem_cache_free(bks_ringbuf_cache, buf);
	}
}

// -------------------------------------------------------------------------
// the write index is always the index of the next open slot. 
// it is one past the last written slot. 
int bks_ringbuf_put(struct BksRingBuffer* ringbuf, unsigned int val)
{
	int rc = 0;
	unsigned int idx; // what the wr index wil be after this put. 


	if (!ringbuf) {
		rc = -1;
	} else {
		spin_lock(&ringbuf->lock);
		idx = ringbuf->wr_idx;
		idx = (idx + 1) % ringbuf->size;

		// if, as a result of this put, the read index and the write index 
		// would coincide, then the queue is full and we need to drop
		// one item from the queue. this is accomplished by advancing 
		// the read index to the next slot.
		if (idx == ringbuf->rd_idx) {
			ringbuf->rd_idx = (idx + 1) % ringbuf->size;
		}

		ringbuf->base[ringbuf->wr_idx] = val;
		ringbuf->wr_idx = idx;

		spin_unlock(&ringbuf->lock);

		rc = 0;// success.
	}
	return rc;
}


// -------------------------------------------------------------------------
// if there is anything in the queue to get, the next value is removed from 
// the queue and returned in the out param, val. 
// returns:
//   1 if the queue is non empty and an element was successfully removed.
//   0 if the queue is empty
//  -1 if passed null pointers.
int bks_ringbuf_get(struct BksRingBuffer* ringbuf, int* val)
{
	int rc = 0;
	unsigned int idx;
	unsigned long flags;

	if (!ringbuf || !val) {
		rc = -1; // an error
	} else {

		spin_lock_irqsave(&ringbuf->lock, flags);

		idx = ringbuf->rd_idx;
		if (idx != ringbuf->wr_idx) {
			// the queue is not empty
			*val = ringbuf->base[idx];
			ringbuf->rd_idx = (idx + 1) % ringbuf->size;
			rc = 1; // returned one element. 
		} else {
			// the queue is empty.
			rc = 0; // returned nothing. 
		}

		spin_unlock_irqrestore(&ringbuf->lock, flags);
	}

	return rc;
}


// -------------------------------------------------------------------------
long bks_ringbuf_copyToUser(struct BksRingBuffer* ringbuf, __user char* buf, unsigned int count)
{
	long bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	unsigned long remain = 0;
	unsigned int ird, iwr;
	unsigned long flags;
	unsigned int ec = 1; 

	printk(KERN_INFO "[bks] ringbuffer copyToUser(), ringbuf: %p, count: %u\n", ringbuf, count);

	if (!ringbuf || !buf) {
		bytes_transferred = -EINVAL;
		goto exit;
	}
	printk(KERN_INFO "ring buffer readindex: %u, write index: %u\n", ringbuf->rd_idx, ringbuf->wr_idx);

	spin_lock_irqsave(&ringbuf->lock, flags);

	ird = ringbuf->rd_idx;
	iwr = ringbuf->wr_idx;

	if (ird == iwr) {
		// empty buffer.
		remain = 0;
	} else if (iwr < ird ) {
		// wrap around - only copy out to the end of buffer.
		remain = ringbuf->size - ird;
	} else { // ird < iwr
		// not wrapped around
		remain = iwr - ird;
	}

	if (count < 0) {
		bytes_to_copy = 0;
	} else if (count < remain) {
		bytes_to_copy = count;
	} else {
		bytes_to_copy = remain;
	}


	if (0 != bytes_to_copy) {
		ec = copy_to_user(buf, &ringbuf->base[ird], bytes_to_copy);
	}

	if (!ec) { // success.
		bytes_transferred = bytes_to_copy;
	} else { // not success.
		bytes_transferred = 0;
	}

	ringbuf->rd_idx = (ird + bytes_transferred) % ringbuf->size;

	spin_unlock_irqrestore(&ringbuf->lock, flags);

exit:
	return bytes_transferred;
}

