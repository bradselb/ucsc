EXE = get_max_children show_path q2 q3 q5 
all : $(EXE)

$(EXE) : % : %.c
	gcc -o $@ -O2 -Wall $<


clean: 
	rm -f $(EXE)


