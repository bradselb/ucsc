#ifndef LAB3_FILEOPS_H
#define LAB3_FILEOPS_H


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brad Selbrede");

// fwd declare some of the pointers used.
struct inode;
struct file;
struct vm_area_struct;
struct poll_table_struct;

// the actual function declarations.
int bks_open(struct inode* inode, struct file* file);
int bks_close(struct inode* inode, struct file* file);

ssize_t bks_read(struct file* file, char* __user buf, size_t count, loff_t* offset);
ssize_t bks_write(struct file* file, const char* __user buf, size_t count, loff_t* offset);

int bks_mmap(struct file* file, struct vm_area_struct* vma);

loff_t bks_llseek(struct file*, loff_t, int);
unsigned int bks_poll(struct file*, struct poll_table_struct*);
long bks_ioctl(struct file *file, unsigned int request, unsigned long arg);
#endif
