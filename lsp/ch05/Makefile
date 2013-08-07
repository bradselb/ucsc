EXE=mysh
LIB=libmysh.a
OBJS=mysh.o
LIBOBJS=cat.o ls.o wc.o

CFLAGS += -Wall -O2

all : $(EXE) $(LIB)

# can build the lib independent of the execuatable
lib : $(LIB)

$(EXE) : $(OBJS) $(LIB)
	$(CC) -o $@ $^

$(LIB) : $(LIBOBJS)
	$(AR) r $@ $^
	# rm $^

%.o : %.c
	$(CC) -c $(CFLAGS) $<


clean :
	rm -f $(OBJS) $(LIBOBJS) $(LIB) $(EXE)  
