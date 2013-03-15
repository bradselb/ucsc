#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h> // struct file_operations
#include <linux/sched.h>
#include <linux/cdev.h>
#include <asm/uaccess.h> // put_user
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/device.h> // dynamic device node creation     

static char my_devname[]= "poll_dev";
static int my_major = 0;
static struct cdev* my_cdev = 0;
static struct class* my_devclass = 0;

#define N 24
static char rbuf[N];
static int  ir, iw; // where to read and where to write
static struct semaphore sema; // protects ir, iw

static int  use_count;
static spinlock_t lock; // protects use_count

static DECLARE_WAIT_QUEUE_HEAD(qin);  // Wait queue for read event
static DECLARE_WAIT_QUEUE_HEAD(qout); // Wait queue for write event



// --------------------------------------------------------------------------
static int is_buffer_empty(void)
{
    int v = 0;
    v = (ir == iw);
    return v;
}

// --------------------------------------------------------------------------
static int is_buffer_full(void)
{
    int v = 1;
    v = (((iw + 1) % N) == ir);
    return v;
}

// --------------------------------------------------------------------------
static unsigned int device_poll(struct file* filp, poll_table* wait)
{
    unsigned int mask = 0;

    poll_wait(filp, &qin, wait);
    poll_wait(filp, &qout, wait);

    if (down_interruptible(&sema)) {
        return -ERESTARTSYS;
    }

    if (!is_buffer_empty()) {
        printk(KERN_INFO "%s - POLLIN EVENT  ir=%d, iw=%d\n", my_devname,ir,iw);
        mask |= POLLIN | POLLRDNORM;
    }

    if (!is_buffer_full()) {
        printk(KERN_INFO "%s - POLLOUT EVENT  ir=%d, iw=%d\n", my_devname,ir,iw);
        mask |= POLLOUT | POLLWRNORM;
    }

    up(&sema);

    return mask;
}


#if 0
goto
#endif

// --------------------------------------------------------------------------
static ssize_t device_read(struct file* filp, char* buffer, size_t len, loff_t* offs)
{
    unsigned int i = 0;
    const int is_non_blocking = (0 != (filp->f_flags & O_NONBLOCK));

    if (is_non_blocking) {
        // this is the non blocking case. no sleeping allowed!
        if (down_trylock(&sema)) {
            // did not get the semaphore.
            return -EAGAIN; // or should it be -ERESTARTSYS ?
        } else if (is_buffer_empty()) {
            // acquired the semaphore but, buffer is empty
            up(&sema);
            return -EAGAIN;
        }

    } else {
        // this is not non-blocking...we can sleep.
        printk(KERN_INFO " %s read - blocking\n", my_devname);

        // read access of the global variables ir and iw though the function
        // is_buffer_empty() is unprotected. However, write access is always
        // protected by the semaphore.
        wait_event_interruptible_exclusive(qin, !is_buffer_empty());

        // roused from sleep by a signal?
        if (signal_pending(current)) {
            return -ERESTARTSYS;
        }

        if (down_interruptible(&sema)) {
            return -ERESTARTSYS;
        }
    }
    // we hold the semaphore and the buffer is not empty.

    while (i < len && !is_buffer_empty()) {
        put_user(rbuf[ir], buffer++);
        ir = (ir + 1) % N;
        ++i;
    }

    printk(KERN_INFO " %s read - %d bytes\n", my_devname, i);

    up(&sema);

    // we just took something out of the buffer so, wake up processes waiting to write
    if (0 < i) {
        wake_up_interruptible(&qout);
    }

    return i;
}


// --------------------------------------------------------------------------
static ssize_t device_write(struct file* filp, const char* buffer, size_t len, loff_t* offs)
{
    unsigned int i = 0; // number of bytes written
    const int is_non_blocking = (0 != (filp->f_flags & O_NONBLOCK));

    if (is_non_blocking) {
        // this is the non blocking casae. no sleeping allowed!
        if (down_trylock(&sema)) {
            // did not get the semaphore.
            return -EAGAIN; // or should it be -ERESTARTSYS ?
        } else if (is_buffer_full()) {
            // acquired the semaphore but, buffer is full
            up(&sema);
            return -EAGAIN;
        }

    } else {
        printk(KERN_INFO " %s write - blocking\n", my_devname);

        wait_event_interruptible_exclusive(qout, !is_buffer_full());

        // roused from sleep by a signal?
        if (signal_pending(current)) {
            return -ERESTARTSYS;
        }

        if (down_interruptible(&sema)) {
            return -ERESTARTSYS;
        }
    }
    // we hold the semaphore and the buffer is not full.

    while (i < len && !is_buffer_full()) {
        get_user(rbuf[iw], buffer++);
        iw = (iw + 1) % N;
        ++i;
    }

    printk(KERN_INFO " %s write - %d bytes\n", my_devname, i);

    up(&sema);

    // Wake up readers
    if (0 < i) {
        wake_up_interruptible(&qin);
    }

    return i;
}


// --------------------------------------------------------------------------
static int device_open(struct inode* inode, struct file* filp)
{
    int use;

    spin_lock(&lock);
    if (0 == use_count) {
        ir = 0;
        iw = 0;
    }
    ++use_count;
    use = use_count;
    spin_unlock(&lock);

    printk(KERN_INFO " %s open - count: %d\n", my_devname, use);

    return 0;
}


// --------------------------------------------------------------------------
static int device_release(struct inode* inode, struct file* filp)
{
    int use;

    spin_lock(&lock);
    --use_count;
    use = use_count;
    spin_unlock(&lock);

    printk(KERN_INFO " %s close - count: %d\n", my_devname, use);

    return 0;
}


// --------------------------------------------------------------------------
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
    .poll = device_poll,
    .open = device_open,
    .release = device_release,
};


// --------------------------------------------------------------------------
static int __init my_init(void)
{
    int rc = 0;
    dev_t dev;
    int first_minor = 0;
    int minor_count = 1;

    spin_lock_init(&lock);
    use_count = 0;

    // initialize stuff related to the buffer.
    memset(rbuf, 0, sizeof rbuf);
    sema_init(&sema, 1);
    ir = 0;
    iw = 0;

    rc = alloc_chrdev_region(&dev, first_minor, minor_count, my_devname);
    if (rc < 0) {
        my_major = 0;
        goto exit;
    }

    my_major = MAJOR(dev);

    printk(KERN_INFO " %s init - major number: %d\n", my_devname, my_major);

    my_cdev = cdev_alloc();
    if (!my_cdev) {
        // fail.
        goto exit;
    }
    cdev_init(my_cdev, &fops);
    my_cdev->owner = THIS_MODULE;

    rc = cdev_add(my_cdev, dev, 1);
    if (rc < 0) {
        goto exit;
    }

    my_devclass = class_create(THIS_MODULE, my_devname);
    if (IS_ERR(my_devclass)) {
        my_devclass = 0;
        goto exit;
    }
    device_create(my_devclass, 0, dev, 0, my_devname, 0);


exit:
    return rc;
}


// --------------------------------------------------------------------------
static void __exit my_exit(void)
{
    printk(KERN_INFO " %s exit.\n", my_devname);

    if (my_devclass) {
        device_destroy(my_devclass, MKDEV(my_major,0));
        class_destroy(my_devclass);
    }

    if (my_cdev) {
        cdev_del(my_cdev);
    }

    if (my_major) {
        unregister_chrdev_region(MKDEV(my_major,0), 1);
    }
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Brad Selbrede");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Description");

