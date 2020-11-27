CC=gcc
CFLAGS=-Wall -Iincludes -Wextra -std=gnu99 -ggdb
LDLIBS=-lcrypto

all: cli

cli.o:
	$(CC) $(CFLAGS) cli.c cli.h shared.h -c $(LDLIBS)

cli: cli.o
	$(CC) $(CFLAGS) -o cli cli.o $(LDLIBS)

clean:
	rm -rf *~ *.o
