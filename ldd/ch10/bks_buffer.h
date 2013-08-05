#if !defined BKS_BUFFER_H
#define BKS_BUFFER_H

struct BksBuffer; // a declaration of existence.

int bksbuffer_init(void);
void bksbuffer_cleanup(void);

struct BksBuffer* bksbuffer_allocate(void);
void bksbuffer_free(struct BksBuffer*);

int bksbuffer_getBufferSize(struct BksBuffer*);
int bksbuffer_getContentLength(struct BksBuffer*);

long bksbuffer_copyToUser(struct BksBuffer*, __user char* buf, unsigned int count);
long bksbuffer_copyFromUser(struct BksBuffer*, __user const char* buf, unsigned int count);

loff_t bksbuffer_lseek(struct BksBuffer* bksbuf, loff_t offset, int whence);

#endif //!defined BKS_BUFFER_H
