LIBNAME = mysh
LIB = lib$(LIBNAME).a
SOURCES =
SOURCES = $(wildcard *.c)
OBJS := $(SOURCES:%.c=%.o)

CFLAGS += -Wall -O2 -std=gnu99

lib : $(LIB)

$(LIB) : $(OBJS)
	$(AR) cr $@ $^

%.o : %.c
	$(CC) -c $(CFLAGS) $<


clean :
	@rm -f $(OBJS) $(LIB)  
