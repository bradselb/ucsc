SERVER = server
CLIENT = client

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=%.o)

CLIENT_OBJ = $(CLIENT).o
SERVER_OBJ = $(SERVER).o

CLIENT_OBJS = $(filter-out $(SERVER_OBJ), $(OBJECTS))
SERVER_OBJS = $(filter-out $(CLIENT_OBJ), $(OBJECTS))


CFLAGS += -Wall -O2 -std=gnu99 -I../lib

all : $(CLIENT) $(SERVER)

# as a convenience, you can build the lib from here
# and the lib needs to be built before the exe.
lib : 
	$(MAKE) -C ../lib $@

help: 
	@echo 'the following make targets exist...'
	@echo '    all:     builds the library, compiles all files in this directory and links the executable'
	@echo '    lib:     builds the library'
	@echo '  clean:     deletes all the build products from this directory'

$(CLIENT) : $(CLIENT_OBJS)
	$(CC) -o $@  $^

$(SERVER) : $(SERVER_OBJS)
	$(CC) -o $@  $^

%.o : %.c
	$(CC) -c $(CFLAGS) $<

clean :
	@rm -f $(OBJECTS) $(CLIENT) $(SERVER)
	$(MAKE) -C ../lib $@
