#if !defined(KBUFFER_H)
#define KBUFFER_H

#include <linux/ioctl.h>

// 218 == 0xda 
#define KBUFFER_MAJOR_NUMBER  (218)


// a sruct to pass in som econtext info 
// not really used. just wanted to see how this works
struct kbuffer_ctx {
	int foo;
	int bar;
};

// define two ioctl numbers
// one to get the current string length and
// one to reinitialize the buffer. 

#define KBUFFER_GET_STRING_LENGTH 	_IO(KBUFFER_MAJOR_NUMBER, 1)
#define KBUFFER_REINITIALIZE		_IOR(KBUFFER_MAJOR_NUMBER, 2, int)
#define KBUFFER_RESIZE  			_IOW(KBUFFER_MAJOR_NUMBER, 3, int)
#define KBUFFER_UPDATE_CTX			_IOW(KBUFFER_MAJOR_NUMBER, 4, struct kbuffer_ctx)
#endif // !defined(KBUFFER_H)
