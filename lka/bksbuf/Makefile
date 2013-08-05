obj-m += bksbuf.o

MY_KERNEL_SRC_DIR = /lib/modules/`uname -r`/source

module:
	make -C ${MY_KERNEL_SRC_DIR}  M=${PWD} modules

demo:demo.c
	gcc -o demo demo.c

clean:
	make -C ${MY_KERNEL_SRC_DIR}  M=${PWD} clean
	rm -f demo
