#ifndef LAB3_FILEOPS_H
#define LAB3_FILEOPS_H


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");


#define device_count 1

int lab3_initBuffers(void);
void lab3_freeBuffers(void);


int lab3_open(struct inode *inode, struct file* file);
int lab3_close(struct inode *inode, struct file *file);
//long lab3_ioctl(struct file *, unsigned int , unsigned long);

ssize_t lab3_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
ssize_t lab3_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

#endif
