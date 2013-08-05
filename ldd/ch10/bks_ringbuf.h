#if !defined BKS_RINGBUF_H
#define BKS_RINGBUF_H

struct BksRingBuffer;

int bks_ringbuf_init(void);
void bks_ringbuf_cleanup(void);

struct BksRingBuffer* bks_ringbuf_allocate(void);
void bks_ringbuf_free(struct BksRingBuffer* buf);

int bks_ringbuf_put(struct BksRingBuffer* ringbuf, unsigned int val);
int bks_ringbuf_get(struct BksRingBuffer* ringbuf, int* val);

long bks_ringbuf_copyToUser(struct BksRingBuffer* ringbuf, __user char* buf, unsigned int count);

#endif // !defined BKS_RINGBUF_H
