obj-m := lab3.o
lab3-objs := lab3_main.o lab3_fileops.o

modules:
	make -C /home/brad/src/linux-2.6.32.59 M=${PWD} modules

demo:demo.c
	gcc -o demo demo.c

clean:
	make -C /home/brad/src/linux-2.6.32.59 M=${PWD} clean
	rm -f demo demo.o
