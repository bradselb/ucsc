EXE=mysh
OBJ=mysh.o
LIBNAME=mysh
LIB=lib$(LIBNAME).a
LIBOBJS=cat.o ls.o wc.o

CFLAGS += -Wall -O2

all : $(EXE) $(LIB)

# can build the lib independent of the execuatable
lib : $(LIB)

$(EXE) : $(OBJ) $(LIB)
	$(CC) -o $@ $(OBJ) -L. -l$(LIBNAME)

$(LIB) : $(LIBOBJS)
	$(AR) cr $@ $^

%.o : %.c
	$(CC) -c $(CFLAGS) $<


clean :
	@rm -f $(OBJS) $(LIBOBJS) $(LIB) $(EXE)  
