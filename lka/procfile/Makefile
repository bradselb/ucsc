obj-m += bks_procfile.o

MY_KERNEL_SRC_DIR = /lib/modules/`uname -r`/source

module:
	make -C ${MY_KERNEL_SRC_DIR}  M=${PWD} modules

clean:
	make -C ${MY_KERNEL_SRC_DIR}  M=${PWD} clean
