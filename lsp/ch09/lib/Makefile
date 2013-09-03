LIBNAME = mysh
LIB = lib$(LIBNAME).a

SOURCES = $(wildcard *.c)
OBJS := $(SOURCES:%.c=%.o)

CFLAGS += -Wall -O2 -std=gnu99


all : $(LIBNAME)

$(LIBNAME) : $(LIB)

$(LIB) : $(OBJS)
	$(AR) cr $@ $^

%.o : %.c
	$(CC) -c $(CFLAGS) $<


clean :
	@rm -f $(OBJS) $(LIB)  
