SERVER = server
CLIENT = client
LIBNAME = mysh

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=%.o)

CLIENT_OBJ = $(CLIENT).o
SERVER_OBJ = $(SERVER).o

CLIENT_OBJS = $(filter-out $(SERVER_OBJ), $(OBJECTS))
SERVER_OBJS = $(filter-out $(CLIENT_OBJ), $(OBJECTS))

MYLIBDIR = ./lib

CFLAGS += -Wall -O2 -std=gnu99 -I$(MYLIBDIR)

all : $(LIBNAME) $(CLIENT) $(SERVER)

lib : $(LIBNAME)

$(LIBNAME) : 
	$(MAKE) -C $(MYLIBDIR) $@
	@rm -f $(CLIENT)
	@rm -f $(SERVER)

help: 
	@echo 'the following make targets exist...'
	@echo '    all:     builds the library, compiles all files in this directory and links the executable'
	@echo '    lib:     builds the library'
	@echo '  clean:     deletes all the build products'

$(CLIENT) : $(LIBNAME) $(CLIENT_OBJS)
	$(CC) -o $@ -L$(MYLIBDIR) $(CLIENT_OBJS) -l$<

$(SERVER) : $(LIBNAME) $(SERVER_OBJS)
	$(CC) -o $@ -L$(MYLIBDIR) $(SERVER_OBJS)  -l$<

%.o : %.c
	$(CC) -c $(CFLAGS) $<

clean :
	@rm -f $(OBJECTS) $(CLIENT) $(SERVER)
	$(MAKE) -C $(MYLIBDIR) $@
