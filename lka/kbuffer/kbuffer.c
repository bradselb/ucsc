//#include <linux/init.h>
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h> // kmalloc(), free()
#include <linux/fs.h>  // file_operations
//#include <linux/string.h>  // strncpy(), strnlen()
//#include <linux/errno.h>
//#include <linux/types.h>
//#include <linux/proc_fs.h>
//#include <linux/fcntl.h>
//#include <asm/system.h> 
#include <asm/uaccess.h>


#include "kbuffer.h" // my ioctl numbers 


MODULE_LICENSE("GPL");


// -------------------------------------------------------------------------
// fwd decl's
int bks_buffer_init(void);
void bks_buffer_exit(void);

int bks_buffer_open(struct inode *inode, struct file* file);
int bks_buffer_close(struct inode *inode, struct file *file);
long bks_buffer_ioctl(struct file *, unsigned int , unsigned long);

ssize_t bks_buffer_read(struct file *file, char *buf, size_t count, loff_t *offset);
ssize_t bks_buffer_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

// -------------------------------------------------------------------------
// globals
static int g_major_number = KBUFFER_MAJOR_NUMBER;

static const char *g_default = "Welcome to UCSC Second Assignment";

static unsigned int g_bufsize = 1024; // initial size but not fixed
static char *g_buffer; // the buffer is dynamically allocated
static char *g_bufptr; // where we are in the buffer.
static int g_open = 0; // is the file open? 



struct file_operations bks_buffer_fops = {
  read: bks_buffer_read,
  write: bks_buffer_write,
  open: bks_buffer_open,
  unlocked_ioctl: bks_buffer_ioctl,
  release: bks_buffer_close
};


// -------------------------------------------------------------------------
module_init(bks_buffer_init);
module_exit(bks_buffer_exit);

// -------------------------------------------------------------------------
// this fctn gets called when the module is loaded. 
int bks_buffer_init() 
{
	int rc;
	printk(KERN_INFO "bks_buffer_init()\n"); 


	g_buffer = 0;
	g_bufptr = g_buffer;
	g_open = 0;

	rc = register_chrdev(g_major_number, "bks_buffer", &bks_buffer_fops); 
	if (rc < 0) {
		printk(KERN_ERR "bks_buffer_init(): attempt to register character device number %d failed.\n", g_major_number);
		// rc = as set by call to register_chrdev(), above.
	} else if (!(g_buffer = kmalloc(g_bufsize, GFP_KERNEL))) {
		printk(KERN_INFO "bks_buffer_init(): could not allocate a buffer of size %d\n", g_bufsize); 
		bks_buffer_exit(); 
		rc = -ENOMEM;
	} else {
		// success 
		memset(g_buffer, 0, g_bufsize);
		rc = 0;
	}

	return rc;
}

// -------------------------------------------------------------------------
// this gets called when the module is unloaded. 
void bks_buffer_exit()
{
	printk(KERN_INFO "bks_buffer_exit()\n");

	unregister_chrdev(g_major_number, "bks_buffer");
	if (g_buffer) 
		kfree(g_buffer);
}

// -------------------------------------------------------------------------
// this gets called when user program calls fopen()
int bks_buffer_open(struct inode* inode, struct file* file)
{
	int rc = -1;

	printk(KERN_INFO "bks_buffer_open()\n"); 
 
	if (g_open) { 
		rc = -EBUSY;
	} else if (!g_buffer) {
		rc = -1; //? 
	} else {
		strncpy(g_buffer, g_default, g_bufsize - 1);
		g_open = 1;
		rc = 0;
	}
	return rc;
}

// -------------------------------------------------------------------------
// called by fclose()
int bks_buffer_close(struct inode* inode, struct file* file)
{
	int rc = 0;

	printk(KERN_INFO "bks_buffer_close()\n"); 

	g_open = 0;
	if ( g_buffer ) 
		memset(g_buffer, 0, g_bufsize);

	return rc;
}

// -------------------------------------------------------------------------
long bks_buffer_ioctl(struct file *file, unsigned int request, unsigned long arg)
{
	int rc = 0;
	const int dir = _IOC_DIR(request);
	const int type = _IOC_TYPE(request);
	const int number = _IOC_NR(request);
	const int size = _IOC_SIZE(request);

	//int* userdata = 0;
	struct kbuffer_ctx ctx;
	int string_length = 0;

	string_length = strnlen(g_buffer, g_bufsize-1);

	printk(KERN_INFO "bks_buffer_ioctl(f, %u, %lu)\n", request, arg);
	printk(KERN_INFO "bks_buffer_ioctl(), dir: %d, type: %d, number: %d, size: %d\n", dir, type, number, size);

	if (KBUFFER_GET_STRING_LENGTH == request) {
		if (!arg) {
			rc = string_length;
		} else {
			//userdata = (int*) arg;
			copy_to_user((int*)arg, &string_length, sizeof(int));
			rc = 0;
		}
	} else if (KBUFFER_REINITIALIZE == request) {
		memset(g_buffer, 0, g_bufsize);
		strncpy(g_buffer, g_default, g_bufsize-1);
		rc = 0;
	} else if (KBUFFER_RESIZE == request) {
		if (g_buffer && arg < 4097) {
			kfree(g_buffer);
			g_bufsize = arg;
			// should do some error checking here...
			g_buffer = kmalloc(g_bufsize, GFP_KERNEL);
			strncpy(g_buffer, g_default, g_bufsize-1);
			rc = 0;
		} else {
			rc = -1;
		}
	} else if (KBUFFER_UPDATE_CTX == request) {
		if (!arg) {
			rc = -1;
		} else {
			copy_from_user(&ctx, (struct kbuffer_ctx*)arg, sizeof(struct kbuffer_ctx));
			printk(KERN_INFO "foo: %d, bar: %d\n", ctx.foo, ctx.bar);
			rc = 0;
		}
	} else {
		rc = -1;
	}


	return rc;
}

// -------------------------------------------------------------------------
// just give the user the whole message every time, ignoring offset, etc.
ssize_t bks_buffer_read(struct file *file, char *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	int ec = -1;

	printk(KERN_INFO "bks_buffer_read()\n"); 

	if (count < g_bufsize) {
		bytes_to_copy = count;
	} else {
		bytes_to_copy = g_bufsize;
	}

	ec = copy_to_user(buf, g_buffer, bytes_to_copy);
	if (!ec) {
		bytes_transferred = bytes_to_copy;;
	}
	//printk(KERN_INFO "bks_buffer_read(): byte transfer count is: %ld\n", bytes_transferred);

	return bytes_transferred;
}


// -------------------------------------------------------------------------
// 
ssize_t bks_buffer_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	ssize_t bytes_transferred = 0;
	size_t bytes_to_copy = 0;
	int ec;

	printk(KERN_INFO "bks_buffer_write()\n"); 

	if (g_bufsize < count) {
		bytes_to_copy = g_bufsize;
	} else {
		bytes_to_copy = count;
	}

	ec = -1;
	if (buf && bytes_to_copy) {
		memset(g_buffer, 0, g_bufsize);
		ec = copy_from_user(g_buffer, buf, bytes_to_copy);
	}

	if (!ec) { 
		bytes_transferred = bytes_to_copy;
	} else {
		bytes_transferred = 0;
	}

	return bytes_transferred;
}



