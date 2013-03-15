#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/memory.h>
#include <linux/mm_types.h>
#include <linux/mm.h>

#include "bks_procfile.h"
#include "bks_context.h" // struct bks_context
#include "bks_mmap.h"  // bks_procfile_read_km(), bks_procfile_read_vm()

// -------------------------------------------------------------------------
// forward decl's
static int bks_procfile_read(char* buf, char** start, off_t offset, int bufsize, int* eof, void* data);


// -------------------------------------------------------------------------
// globals
static const char* BKS_PROC_DIR_NAME = "bks";



// -------------------------------------------------------------------------
int bks_create_proc_dir_entry(void* private_data)
{
    int rc = 0;
    struct proc_dir_entry* entry = 0;

    printk(KERN_INFO "[bks] bks_create_proc_dir_entry()\n");

    entry = create_proc_read_entry(BKS_PROC_DIR_NAME, 0, 0, bks_procfile_read, private_data);

    if (!entry) {
        printk(KERN_ERR "[bks] unable to create dir /proc/%s\n", BKS_PROC_DIR_NAME);
        rc = -ENOMEM;
    }

    return rc;
}


// -------------------------------------------------------------------------
void bks_remove_proc_dir_entry(void)
{
    printk(KERN_INFO "[bks] bks_remove_proc_dir_entry()\n");

    remove_proc_entry(BKS_PROC_DIR_NAME, 0);

}




// -------------------------------------------------------------------------
static int bks_procfile_read(char* buf, char** start, off_t offset, int bufsize, int* eof, void* data)
{
    int count = 0;
    int nr = -1;
    struct bks_context* priv = (struct bks_context*)data;

    if (priv) {
        nr = priv->minor;
    }

    printk(KERN_INFO "%s: procfile_read(), minor: %d\n", "bks", nr);


    if (0 == nr) {
        // minor number zero will use the space allocated by kmalloc
        count = bks_procfile_read_km(buf, start, offset, bufsize,  eof, data);
    } else if (1 == nr) {
        // minor number one will use the space allocated by vmalloc
        count = bks_procfile_read_vm(buf, start, offset, bufsize,  eof, data);
    }

    // either way, tell caller that this is EOF.
    if (eof) {
        *eof = -1;
    }
    return count;
}



