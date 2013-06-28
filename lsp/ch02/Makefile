EXE = ls cat wc
all : $(EXE)

$(EXE) : % : %.c
	gcc -o $@ -O2 -Wall $<


clean: 
	rm -f $(EXE)


