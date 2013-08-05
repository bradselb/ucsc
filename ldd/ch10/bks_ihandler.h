#if !defined BKS_IHANDLER_H
#define BKS_IHANDLER_H

#include <linux/interrupt.h>

irqreturn_t bks_ihandler(int irq, void* dev_id);

#endif //!defined BKS_IHANDLER_H
