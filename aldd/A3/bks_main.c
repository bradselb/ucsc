#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h> // struct file_operations
#include <linux/mm.h>
#include <linux/slab.h> // kmalloc()
#include <linux/cdev.h>
#include <asm/uaccess.h> // put_user
#include <linux/device.h> // dynamic device node creation     

#include "bks_context.h" // struct bks_context
#include "bks_mmap.h"
#include "bks_procfile.h"

MODULE_AUTHOR("Brad Selbrede");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Assignment #3 - mmap");


// --------------------------------------------------------------------------
static char* bks_devname = "bks_mmap";
static struct cdev* bks_cdev = 0;
static struct class* bks_devclass = 0;
static int bks_minor_count = 2;


// --------------------------------------------------------------------------
static int bks_major = 0;
module_param_named(major, bks_major, int, S_IRUGO);
MODULE_PARM_DESC(major, "device major number");

// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
static int bks_open(struct inode* inode, struct file* file)
{
    int rc = 0;
    int nr = MINOR(inode->i_rdev);
    struct bks_context* priv = kmalloc(sizeof *priv, GFP_KERNEL);

    printk(KERN_INFO "%s: open() minor: %d\n", bks_devname, nr);
    if (!priv) {
        rc = -ENOMEM;
    } else {
        priv->minor = nr;
        file->private_data = priv;
    }
    // the instructions for this assignment say to allocate the buffer
    // in the open method (dependent upon which minor was opened).
    // and since  we cannot create the procfile until the buffer is allocated,
    // we cannot create the proc file until the device is opened.
    // this is clumsy, non-standard and not terribly useful but, here it is.
    bks_create_proc_dir_entry(priv);

    return rc;
}

// -------------------------------------------------------------------------
static int bks_close(struct inode* inode, struct file* file)
{
    int rc = 0;
    int nr = MINOR(inode->i_rdev);
    struct bks_context* priv = (struct bks_context*)file->private_data;

    printk(KERN_INFO "%s: close(), minor: %d\n", bks_devname, nr);

    bks_remove_proc_dir_entry();

    if (priv) {
        kfree(priv);
    }

    return rc;
}


// -------------------------------------------------------------------------
// the default mmap() function defers to the mmap function for the specific
// memory area.
int bks_mmap(struct file* file, struct vm_area_struct* vma)
{
    int rc = 0;
    int nr = -1;
    struct bks_context* priv = (struct bks_context*)file->private_data;

    if (priv) {
        nr = priv->minor;
    }

    printk(KERN_INFO "%s: mmap(), minor: %d\n", bks_devname, nr);

    if (0 == nr) {
        // minor number zero will use the space allocated by kmalloc
        bks_kmalloc_mmap(file, vma);
    } else if (1 == nr) {
        // minor number one will use the space allocated by vmalloc
        bks_vmalloc_mmap(file, vma);
    } else {
        rc = -EINVAL;
    }

    return rc;
}

// -------------------------------------------------------------------------
// from fs.h
//struct file_operations
//{
//    struct module *owner;
//    loff_t (*llseek) (struct file *, loff_t, int);
//    ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
//    ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
//    ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
//    ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
//    int (*readdir) (struct file *, void *, filldir_t);
//    unsigned int (*poll) (struct file *, struct poll_table_struct *);
//    int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long);
//    long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
//    long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
//    int (*mmap) (struct file *, struct vm_area_struct *);
//    int (*open) (struct inode *, struct file *);
//    int (*flush) (struct file *, fl_owner_t id);
//    int (*release) (struct inode *, struct file *);
//    int (*fsync) (struct file *, struct dentry *, int datasync);
//    int (*aio_fsync) (struct kiocb *, int datasync);
//    int (*fasync) (int, struct file *, int);
//    int (*lock) (struct file *, int, struct file_lock *);
//    ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
//    unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
//    int (*check_flags)(int);
//    int (*flock) (struct file *, int, struct file_lock *);
//    ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
//    ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
//    int (*setlease)(struct file *, long, struct file_lock **);
//};
// --------------------------------------------------------------------------
static struct file_operations bks_fops = {
    .owner = THIS_MODULE,
    .open = bks_open,
    .release = bks_close,
    //.read = bks_read,
    //.write = bks_write,
    .mmap = bks_mmap,
};


// --------------------------------------------------------------------------



// --------------------------------------------------------------------------
static int __init bks_init(void)
{
    int rc = 0;
    dev_t dev;
    int first_minor = 0;

    bks_init_kmem();
    bks_init_vmem();

    if (bks_major) {
        dev = MKDEV(bks_major, first_minor);
        rc = register_chrdev_region(dev, bks_minor_count,  bks_devname);
    } else {
        rc = alloc_chrdev_region(&dev, first_minor, bks_minor_count, bks_devname);
    }


    if (rc < 0) {
        bks_major = 0;
        goto exit;
    }

    bks_major = MAJOR(dev);

    printk(KERN_INFO "%s: init(),  major number: %d\n", bks_devname, bks_major);

    bks_cdev = cdev_alloc();
    if (!bks_cdev) {
        // fail.
        rc = -1;
        goto exit;
    }
    cdev_init(bks_cdev, &bks_fops);
    bks_cdev->owner = THIS_MODULE;

    rc = cdev_add(bks_cdev, dev, bks_minor_count);
    if (rc < 0) {
        goto exit;
    }

    bks_devclass = class_create(THIS_MODULE, bks_devname);
    if (IS_ERR(bks_devclass)) {
        bks_devclass = 0;
        rc = PTR_ERR(bks_devclass);
        goto exit;
    }

    for (int i = first_minor; i<bks_minor_count; ++i) {
        device_create(bks_devclass, 0, MKDEV(bks_major, i), 0, "%s%d", bks_devname, i);
    }

exit:
    return rc;
}

// --------------------------------------------------------------------------
static void __exit bks_exit(void)
{
    int first_minor = 0;

    printk(KERN_INFO "%s: exit()\n", bks_devname);

    if (bks_devclass) {
        for (int i = first_minor; i<bks_minor_count; ++i) {
            device_destroy(bks_devclass, MKDEV(bks_major,i));
        }
        class_destroy(bks_devclass);
    }

    if (bks_cdev) {
        cdev_del(bks_cdev);
    }

    if (bks_major) {
        unregister_chrdev_region(MKDEV(bks_major,0), bks_minor_count);
    }

    bks_cleanup_vmem();
    bks_cleanup_kmem();

}


module_init(bks_init);
module_exit(bks_exit);

