EXE = wc hello_threads
all : $(EXE)

$(EXE) : % : %.c
	gcc -o $@ -O2 -Wall -pthread $<


clean: 
	rm -f $(EXE)


