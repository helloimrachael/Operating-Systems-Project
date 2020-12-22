CC = gcc
CFLAGS += -std=c99 -Wall -pedantic -Werror -Wmissing-field-initializers
INCLUDES = -I
OPTFLAGS = -g
LIBS = -lpthread

TARGETS = supervisor  \
		server	\
		client	\

.PHONY: all clean cleanall test
.SUFFIXES: .c .h

all: $(TARGETS)

supervisor: supervisor.c util.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< 

server: server.c util.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LIBS)

client: client.c util.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< 


clean:
	rm -f $(TARGETS)

cleanall: clean
	\rm -f OOB* *.o *.log

test:
	bash ./test.sh
