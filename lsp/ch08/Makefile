EXE = mysh
SOURCES = $(wildcard *.c)
//OBJS = mysh.o mysh_common.o mysh_pipe.o mysh_sysv.o cat.o ls.o wc.o params.o
OBJS := $(SOURCES:%.c=%.o)

CFLAGS += -Wall -O2 -std=gnu99

all : $(EXE)

$(EXE) : $(OBJS)
	$(CC) -o $@ $^

%.o : %.c
	$(CC) -c $(CFLAGS) $<

clean :
	@rm -f $(OBJS) $(EXE)  
