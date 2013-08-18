EXE = mysh
OBJS = mysh.o cat.o ls.o wc.o

CFLAGS += -Wall -O2 -std=gnu99

all : $(EXE)

$(EXE) : $(OBJS)
	$(CC) -o $@ $^

%.o : %.c
	$(CC) -c $(CFLAGS) $<

clean :
	@rm -f $(OBJS) $(EXE)  
