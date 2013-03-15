obj-m := 8139bks.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
EXTRA_CFLAGS += -std=gnu99

all: modules

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
#	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -f *.o core .depend .*.cmd *.ko *.mod.c Module.symvers modules.order
#	rm -rf .tmp_versions

