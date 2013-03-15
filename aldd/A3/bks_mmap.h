#if !defined BKS_MMAP_H
#define BKS_MMAP_H

int bks_init_vmem(void);
void bks_cleanup_vmem(void);
int bks_vmalloc_mmap(struct file* filp, struct vm_area_struct* vma);
int bks_procfile_read_km(char* buf, char** start, off_t offset, int bufsize, int* eof, void* data);

int bks_init_kmem(void);
void bks_cleanup_kmem(void);
int bks_kmalloc_mmap(struct file* filp, struct vm_area_struct* vma);
int bks_procfile_read_vm(char* buf, char** start, off_t offset, int bufsize, int* eof, void* data);

#endif //!defined BKS_MMAP_H
