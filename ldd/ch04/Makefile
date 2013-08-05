obj-m := bks.o
bks-objs := bks_main.o bks_fileops.o bks_procfile.o

modules:
	make -C /home/brad/src/linux-2.6.32.59 M=${PWD} modules

demo:demo.c
	gcc -o demo demo.c

clean:
	make -C /home/brad/src/linux-2.6.32.59 M=${PWD} clean
	rm -f demo demo.o
