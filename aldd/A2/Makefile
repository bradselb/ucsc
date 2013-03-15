obj-m := bks.o
bks-objs := bks_main.o bks_context.o

KVER ?= $(shell uname -r)
KERNELDIR ?= /lib/modules/$(KVER)/build
EXTRA_CFLAGS += -std=gnu99

all: modules setfreq setpattern

setfreq: setfreq.c bks_ioctl.h
	gcc $(EXTRA_CFLAGS) -o $@ $<

setpattern: setpattern.c bks_ioctl.h
	gcc $(EXTRA_CFLAGS) -o $@ $<

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -f *.o core .depend .*.cmd *.ko *.mod.c Module.symvers modules.order
	rm -rf .tmp_versions

