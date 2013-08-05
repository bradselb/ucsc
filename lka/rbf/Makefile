LIB = libRBF.a
DEMO = demo
SRC = rbf.c rbf_help.c slist.c
OBJ = $(SRC:.c=.o)


$(DEMO) : demo.o $(LIB)
	gcc -o $@ $< -lRBF -L.

$(LIB) : $(OBJ)
	ar rc $@ $^


%.o : %.c %.h
	gcc -c -o $@ $<

clean: 
	rm -f *.o $(LIB) $(DEMO)


