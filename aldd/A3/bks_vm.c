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
#include <linux/vmalloc.h>
#include <linux/slab.h>

#include "bks_fill.h"


static char* vmalloc_ptr = 0;

#define LEN (16*1024)

int bks_vmalloc_mmap(struct file* filp, struct vm_area_struct* vma)
{
    int rc = 0;
    long length = vma->vm_end - vma->vm_start;
    unsigned long start = vma->vm_start;
    char* vmalloc_area_ptr = (char*)vmalloc_ptr;
    unsigned long pfn;

    if (LEN < length) {
        rc = -EIO;
        goto exit;
    }

    // Considering vmalloc pages are not contiguous in physical memory
    // You need to loop over all pages and call remap_pfn_range()
    // for each page individuallay. Also, use
    // vmalloc_to_pfn(vmalloc_area_ptr)
    // instead to get the page frame number of each virtual page
    vma->vm_flags |= VM_RESERVED;
    while (length > 0) {
        pfn = vmalloc_to_pfn(vmalloc_area_ptr);
        printk(KERN_INFO "vmalloc_area_ptr: 0x%p \n", vmalloc_area_ptr);

        rc = remap_pfn_range(vma, start, pfn, PAGE_SIZE, vma->vm_page_prot);
        if (rc < 0) {
            break;
        }
        start += PAGE_SIZE;
        vmalloc_area_ptr += PAGE_SIZE;
        length -= PAGE_SIZE;
    }

exit:
    return rc;
}

int bks_init_vmem(void)
{
    int rc = 0;
    unsigned long virt_addr;

    /* Allocate  memory  with vmalloc. It is already page aligned */
    vmalloc_ptr = vmalloc(LEN);
    if (!vmalloc_ptr) {
        printk(KERN_INFO "vmalloc failed\n");
        rc = -ENOMEM;
        goto exit;
    }

    printk(KERN_INFO "vmalloc_ptr: 0x%p, physical Address 0x%08lx)\n", vmalloc_ptr, (unsigned long)virt_to_phys(vmalloc_ptr));

    /* reserve vmalloc memory to make it remapable */
    for (virt_addr = (unsigned long)vmalloc_ptr;
            virt_addr < (unsigned long)vmalloc_ptr + LEN;
            virt_addr += PAGE_SIZE) {
        SetPageReserved(vmalloc_to_page((unsigned long*)virt_addr));
    }

    {
        // a local scope for these convenience variables.
        char s[] = "abcdefghijklmnopqrstuvwxyz";
        unsigned int size = (unsigned int)(LEN);
        bks_fill(vmalloc_ptr, size, s, strlen(s));
    }

exit:
    return rc;
}

void bks_cleanup_vmem(void)
{
    unsigned long virt_addr;
    printk(KERN_INFO "bks_cleanup_vmem()\n");

    for (virt_addr = (unsigned long)vmalloc_ptr;
            virt_addr < (unsigned long)vmalloc_ptr + LEN;
            virt_addr += PAGE_SIZE) {
        // mark all pages un-reserved
        ClearPageReserved(vmalloc_to_page((unsigned long*)virt_addr));
    }
    vfree(vmalloc_ptr);
}


// -------------------------------------------------------------------------
int bks_procfile_read_vm(char* buf, char** start, off_t offset, int bufsize, int* eof, void* data)
{
    int count = 0;


    if (0 < offset) {
        count = 0;
    } else {
        count += snprintf(buf+count, bufsize-count, "%s\n", vmalloc_ptr);
        //count += snprintf(buf+count, bufsize-count, "\n");
        //count += snprintf(buf+count, bufsize-count, "\n");
    }

    return count;
}


