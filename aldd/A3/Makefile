obj-m := bks.o
bks-objs := bks_fill.o bks_km.o bks_main.o bks_procfile.o bks_vm.o

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

