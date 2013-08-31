EXE = client

SOURCES = $(wildcard *.c)
OBJS := $(SOURCES:%.c=%.o)

CFLAGS += -Wall -O2 -std=gnu99 -I../lib

all : lib $(EXE)

# as a convenience, you can build the lib from here
# and the lib needs to be built before the exe.
lib : 
	$(MAKE) -C ../lib $@

help: 
	@echo 'the following make targets exist...'
	@echo '    all:     builds the library, compiles all files in this directory and links the executable'
	@echo '    lib:     builds the library'
	@echo '  clean:     deletes all the build products from this directory'

$(EXE) : $(OBJS)
	$(CC) -o $@ -L../lib $^  -lmysh

%.o : %.c
	$(CC) -c $(CFLAGS) $<

clean :
	@rm -f $(OBJS) $(EXE)  
