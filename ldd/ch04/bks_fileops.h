#ifndef LAB3_FILEOPS_H
#define LAB3_FILEOPS_H


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");


#define device_count 1

int bks_initBuffers(void);
void bks_freeBuffers(void);

int bks_getBufferSize(int nr);
int bks_getBufferContentLength(int nr);

int bks_open(struct inode *inode, struct file* file);
int bks_close(struct inode *inode, struct file *file);
//long bks_ioctl(struct file *, unsigned int , unsigned long);

ssize_t bks_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
ssize_t bks_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

#endif
