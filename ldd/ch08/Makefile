obj-m := bks.o
bks-objs := bks_main.o bks_fileops.o bks_buffer.o bks_procfile.o bks_opencount.o

modules:
	make -C /home/brad/src/linux-2.6.32.59 M=${PWD} modules

demo: demo.c demo_rd.c demo_lseek demo_ioctl
	gcc -o demo demo.c
	gcc -o demo_rd demo_rd.c
	gcc -o demo_lseek demo_lseek.c
	gcc -o demo_ioctl demo_ioctl.c

clean:
	make -C /home/brad/src/linux-2.6.32.59 M=${PWD} clean
