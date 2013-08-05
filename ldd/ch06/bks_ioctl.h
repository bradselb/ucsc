#if !defined BKS_IOCTL_H
#define BKS_IOCTL_H

#include <linux/ioctl.h>

#define BKS_DRIVER_MAGIC (218)

#define BKS_IOCTL_OPEN_COUNT _IOR(BKS_DRIVER_MAGIC, 1, int)
#define BKS_IOCTL_BUFSIZE _IOR(BKS_DRIVER_MAGIC, 2, int)
#define BKS_IOCTL_CONTENT_LENGTH _IOR(BKS_DRIVER_MAGIC, 3, int)

#endif // !defined BKS_IOCTL_H

