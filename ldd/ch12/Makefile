obj-m := bks.o
bks-objs := bks_main.o bks_procfile.o

modules:
	make -C /home/brad/src/linux-2.6.32.59 M=${PWD} modules

clean:
	make -C /home/brad/src/linux-2.6.32.59 M=${PWD} clean
