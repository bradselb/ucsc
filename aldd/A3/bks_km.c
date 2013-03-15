#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/uaccess.h>
//#include <linux/vmalloc.h>
#include <linux/slab.h>

#include "bks_fill.h"

// a multi-page buffer that is allocated by kmalloc.
// this address is not necessarily page aligned.
static char* kmalloc_ptr = 0;
// a pointer to the first page aligned address in the buffer
static char* kmalloc_area = 0;

#define LEN (16*1024) // 16k == 4 pages
#define PAD (2 * PAGE_SIZE)
// bufsize is LEN + PAD.


// -------------------------------------------------------------------------
int bks_kmalloc_mmap(struct file* filp, struct vm_area_struct* vma)
{
    int rc = 0;
    unsigned long start = vma->vm_start;
    unsigned long length = vma->vm_end - vma->vm_start;
    unsigned long pfn = page_to_pfn(virt_to_page(kmalloc_area));

    if (LEN < length) {
        rc = -EIO;
        goto exit;
    }

    vma->vm_flags |= VM_RESERVED;
    rc = remap_pfn_range(vma, start, pfn, length, vma->vm_page_prot);
    if (rc != 0) {
        rc = -EAGAIN;
    }

exit:
    return rc;
}


// -------------------------------------------------------------------------
int bks_init_kmem(void)
{
    int rc;
    unsigned long virt_addr;

    kmalloc_ptr = kmalloc(LEN + PAD, GFP_KERNEL);
    if (!kmalloc_ptr) {
        printk(KERN_INFO "kmalloc failed\n");
        rc =  -ENOMEM;
        goto exit;
    }
    printk(KERN_INFO "kmalloc_ptr: 0x%p \n", kmalloc_ptr);


    // this gets the (page aligned) address of the fist whole page in the allocated buffer.
    // this is not a catastrophy because the allocation included two full pages more
    // than we really need/want. So, we waste some on the front end and add one to the
    // end of the buffer for protection against running past end of buffer.
    kmalloc_area = (char*)(((unsigned long)kmalloc_ptr + PAGE_SIZE -1) & PAGE_MASK);

    printk(KERN_INFO "kmalloc_area: 0x%p, physical Address 0x%lx)\n", kmalloc_area, (unsigned long)virt_to_phys(kmalloc_area));

    /* reserve kmalloc memory as pages to make them remapable */
    for (virt_addr = (unsigned long)kmalloc_area;
            virt_addr < (unsigned long)kmalloc_ptr + LEN; // end of buffer.
            virt_addr += PAGE_SIZE) {
        SetPageReserved(virt_to_page(virt_addr));
    }

    {
        // a local scope for these convenience variables.
        char s[] = "0123456789";
        unsigned int size = (unsigned int)(LEN + PAD);
        bks_fill(kmalloc_ptr, size, s, strlen(s));
    }

exit:
    return rc;
}


// -------------------------------------------------------------------------
void bks_cleanup_kmem(void)
{
    unsigned long virt_addr;
    printk(KERN_INFO "bks_cleanup_kmem()\n");

    for (virt_addr = (unsigned long)kmalloc_area;
            virt_addr < (unsigned long)kmalloc_ptr + LEN;
            virt_addr += PAGE_SIZE) {
        ClearPageReserved(virt_to_page(virt_addr));
    }
    kfree(kmalloc_ptr);
}


// -------------------------------------------------------------------------
int bks_procfile_read_km(char* buf, char** start, off_t offset, int bufsize, int* eof, void* data)
{
    int count = 0;


    if (0 < offset) {
        count = 0;
    } else {
        count += snprintf(buf+count, bufsize-count, "%s\n", kmalloc_area);
        //count += snprintf(buf+count, bufsize-count, "\n");
        //count += snprintf(buf+count, bufsize-count, "\n");
    }

    return count;
}

