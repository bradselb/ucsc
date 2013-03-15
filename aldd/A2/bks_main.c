#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h> // struct file_operations
//#include <linux/sched.h>
#include <linux/slab.h> // kmalloc()
#include <linux/cdev.h>
#include <asm/uaccess.h> // put_user
//#include <linux/wait.h>
//#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/device.h> // dynamic device node creation     
#include <linux/tty.h> // fg_console, MAX_NR_CONSOLES 
#include <linux/kd.h> // KDSETLED 
#include <linux/vt_kern.h>

#include "bks_ioctl.h"
#include "bks_context.h"

MODULE_AUTHOR("Brad Selbrede");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Assignment #2 - using kernel timer functions and work queues.");


// --------------------------------------------------------------------------
static char* bks_devname = "bks_blink";
static int bks_major = 0;
static struct cdev* bks_cdev = 0;
static struct class* bks_devclass = 0;
static struct timer_list bks_timer;
static struct workqueue_struct* bks_wqs = 0;

static struct bks_context* bks_ctx = 0;
static struct tty_driver* bks_ttydriver = 0;

// --------------------------------------------------------------------------
module_param_named(major, bks_major, int, S_IRUGO);
MODULE_PARM_DESC(major, "device major number");

// somebody is just so enamored with the container of macro that
// we are forced to use that silly macro to use a work queue.
static struct work_container {
    struct bks_context* ctx;
    struct work_struct work;
} bks_wc;

// --------------------------------------------------------------------------
static void bks_work_function(struct work_struct* arg)
{
    int state = 0x80; // default to normal led function
    struct work_container* wc = container_of(arg, struct work_container, work);
    struct bks_context* ctx = wc->ctx;

    int rc = bks_get_state(ctx);
    if (-1 < rc) {
        state = rc;
    }

    // on my (2004 vintage) keyboard, the physical arrangement of the LED
    // is such that this bit swapping makes the led positions correspond 
    // to the bit positions.
    state = ((state&4)>>1) | ((state&2)<<1) | (state&1);
    (bks_ttydriver->ops->ioctl)(vc_cons[fg_console].d->vc_tty, 0, KDSETLED, state&0x0f);
}

// -------------------------------------------------------------------------
//                            0,1,2,3,4,5,6,7
static const int lookup9[] = {4,3,1,5,2,6,7,0};

// -------------------------------------------------------------------------
// timer updates the state and schedules work function.
static void bks_timer_function(unsigned long arg)
{
    int state = -1;
    int pattern = -1;
    int freq = HZ;
    int rc;
    struct bks_context* ctx = (struct bks_context*)arg;

    rc = bks_get_state(ctx);
    if (-1 < rc) {
        state = rc;
    }

    rc = bks_get_pattern(ctx);
    if (-1 < rc) {
        pattern = rc;
    }

    rc = bks_get_freq(ctx);
    if (0 < rc) {
        freq = rc;
    }

    switch (pattern) {

    case 0:
        state = 0x80; // restore the LED state to normal.
        break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        if (0 == state) {
            state = pattern;
        } else {
            state = 0;
        }
        break;

    case 8:
        // binary counter
        if (state < 0) {
            state = 0;
        } else {
            state = (state + 1) & 0x07;
        }
        break;


    case 9:
        // a funny state machine.
        if (-1<state && state<8) {
            state = lookup9[state];
        } else {
            state = 0;
        }
        break;



    default: 
        state = 0x80; // restore the LED state to normal.
        break;
    }

    if (-1 < state) {
        rc = bks_set_state(ctx, state);
    }

    // this works just fine but, the homework assignmnet requires that this 
    // call to the tty driver's ioctl be done in a worker thread. 
    //state = ((state&4)>>1) | ((state&2)<<1) | (state&1);
    //(bks_ttydriver->ops->ioctl)(vc_cons[fg_console].d->vc_tty, 0, KDSETLED, state&0x0f);
    queue_work(bks_wqs, &bks_wc.work);

    // restart the timer for this function.

    if (pattern < 8) {
        bks_timer.expires = jiffies + HZ/(freq<<1);
    } else {
        bks_timer.expires = jiffies + HZ/freq;
    }
    add_timer(&bks_timer);
}


// -------------------------------------------------------------------------
//
static int bks_open(struct inode* inode, struct file* file)
{
    int rc = 0;
    int nr = MINOR(inode->i_rdev);

    printk(KERN_INFO "%s: open() minor: %d, f_flags: 0x%08x\n", bks_devname, nr, file->f_flags);

    // use private data to keep track of device context...even though it is global.
    file->private_data = bks_ctx;

    return rc;
}

// -------------------------------------------------------------------------
//
static int bks_close(struct inode* inode, struct file* file)
{
    int rc = 0;
    int nr = MINOR(inode->i_rdev);
    //struct bks_context* ctx = file->private_data;

    printk(KERN_INFO "%s: close(), minor: %d\n", bks_devname, nr);

    return rc;
}


// -------------------------------------------------------------------------
static long bks_ioctl(struct file* file, unsigned int request, unsigned long arg)
{
    long rc = 0;
    int val;
    struct bks_context* ctx = file->private_data;

    const int dir = _IOC_DIR(request);
    const int type = _IOC_TYPE(request);
    const int number = _IOC_NR(request);
    const int size = _IOC_SIZE(request);

    printk(KERN_INFO "%s: ioctl(), request: %u, arg: %lu)\n", bks_devname, request, arg);
    printk(KERN_INFO "%s: ioctl(), data direction: %d, type: %d, number: %d, size: %d\n", bks_devname, dir, type, number, size);

    if (!(ctx && arg)) {
        rc = -EINVAL;
        goto exit; // not much we can do.
    }


    switch (request) {

    case BKS_IOCTL_GET_BLINK_FREQUENCY:
        val = bks_get_freq(ctx);
        rc = copy_to_user((int*)arg, &val, sizeof(int));
        break;

    case BKS_IOCTL_SET_BLINK_FREQUENCY:
        rc = copy_from_user(&val, (int*)arg, sizeof(int));
        if (0 != rc) {
            rc = -EFAULT;
        } else if (val < 1 || 10 < val) {
            rc = -EINVAL;
        } else {
            bks_set_freq(ctx, val);
        }
        break;

    case BKS_IOCTL_GET_BLINK_PATTERN:
        val = bks_get_pattern(ctx);
        rc = copy_to_user((int*)arg, &val, sizeof(int));
        break;

    case BKS_IOCTL_SET_BLINK_PATTERN:
        rc = copy_from_user(&val, (int*)arg, sizeof(int));
        if (0 != rc) {
            rc = -EFAULT;
        } else if (val < 0 || 9 < val) {
            rc = -EINVAL;
        } else {
            bks_set_pattern(ctx, val);
        }
        break;

    case BKS_IOCTL_GET_BLINK_STATE:
        val = bks_get_state(ctx);
        rc = copy_to_user((int*)arg, &val, sizeof(int));
        break;

    default:
        rc = -EINVAL;
        break;

    }

exit:
    return rc;
}

// --------------------------------------------------------------------------
static struct file_operations bks_fops = {
    .owner = THIS_MODULE,
    .open = bks_open,
    .release = bks_close,
    //.read = bks_read,
    //.write = bks_write,
    .unlocked_ioctl = bks_ioctl,
};


// --------------------------------------------------------------------------
static int bks_scan_consoles(void)
{
    int rc = 0;

    for (int i = 0; i < MAX_NR_CONSOLES; i++) {
        if (!vc_cons[i].d) {
            break;
        }
        printk(KERN_INFO "%s: console[%i/%i] #%i, tty %lx\n", bks_devname, i,
               MAX_NR_CONSOLES, vc_cons[i].d->vc_num, (unsigned long)vc_cons[i].d->vc_tty);
    }
    printk(KERN_INFO "%s: finished scanning consoles\n", bks_devname);

    return rc;
}



// --------------------------------------------------------------------------
static int __init bks_init(void)
{
    int rc = 0;
    dev_t dev;
    int first_minor = 0;
    int minor_count = 1;


    if (bks_major) {
        dev = MKDEV(bks_major, first_minor);
        rc = register_chrdev_region(dev, minor_count,  bks_devname);
    } else {
        rc = alloc_chrdev_region(&dev, first_minor, minor_count, bks_devname);
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

    rc = cdev_add(bks_cdev, dev, 1);
    if (rc < 0) {
        goto exit;
    }

    bks_devclass = class_create(THIS_MODULE, bks_devname);
    if (IS_ERR(bks_devclass)) {
        bks_devclass = 0;
        rc = PTR_ERR(bks_devclass);
        goto exit;
    }

    for (int i = first_minor; i<minor_count; ++i) {
        device_create(bks_devclass, 0, dev, 0, "%s%d", bks_devname, i);
    }

    //bks_create_proc_dir_entry();

    //bks_scan_consoles();
    bks_ttydriver = vc_cons[fg_console].d->vc_tty->driver;
    printk(KERN_INFO "%s: tty driver magic %x\n", bks_devname, bks_ttydriver->magic);

    bks_ctx = bks_create_context();

    bks_wc.ctx = bks_ctx;
    bks_wqs = create_singlethread_workqueue("bks_wqs");
    INIT_WORK(&bks_wc.work, bks_work_function);


    // start the timer function.
    init_timer(&bks_timer);
    bks_timer.function = bks_timer_function;
    bks_timer.data = (unsigned long)bks_ctx;
    bks_timer.expires = jiffies + HZ;
    add_timer(&bks_timer);


exit:
    return rc;
}

// --------------------------------------------------------------------------
static void __exit bks_exit(void)
{
    printk(KERN_INFO "%s: exit()\n", bks_devname);

    del_timer(&bks_timer);

    flush_workqueue(bks_wqs);
    destroy_workqueue(bks_wqs);

    bks_free_context(bks_ctx);

    if (bks_ttydriver) {
        (bks_ttydriver->ops->ioctl)(vc_cons[fg_console].d->vc_tty, 0, KDSETLED, 0x80);
    }

    //bks_remove_proc_dir_entry();

    if (bks_devclass) {
        device_destroy(bks_devclass, MKDEV(bks_major,0));
        class_destroy(bks_devclass);
    }

    if (bks_cdev) {
        cdev_del(bks_cdev);
    }

    if (bks_major) {
        unregister_chrdev_region(MKDEV(bks_major,0), 1);
    }
}


module_init(bks_init);
module_exit(bks_exit);

