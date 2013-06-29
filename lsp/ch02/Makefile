EXE = ls cat wc get_max_open redirect
all : $(EXE)

$(EXE) : % : %.c
	gcc -o $@ -O2 -Wall $<


clean: 
	rm -f $(EXE)


