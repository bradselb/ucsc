#if !defined(BKSBUF_H)
#define BKSBUF_H

#include <linux/ioctl.h>

// 218 == 0xda 
#define BKSBUF_MAGIC_NUMBER  (218)


struct bksbuf_addr {
	unsigned long int virt; // the virtual address of the page
	unsigned long int phys; // the physical address of the page
};

// define two ioctl numbers
// one to get the current string length and
// one to get the address of the page. 

#define BKSBUF_GET_STRLEN  _IOR(BKSBUF_MAGIC_NUMBER, 1, int)
#define BKSBUF_GET_ADDRESS _IOR(BKSBUF_MAGIC_NUMBER, 2, struct bksbuf_addr)

#endif // !defined(BKSBUF_H)
