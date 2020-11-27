CC=gcc
CFLAGS=-Wall -Iincludes -Wextra -std=gnu99 -ggdb
LDLIBS=-lcrypto

all: cli

torrent_runtime.o:
	$(CC) $(CFLAGS) torrent_runtime.c -c $(LDLIBS)

cli.o:
	$(CC) $(CFLAGS) cli.c cli.h shared.h -c $(LDLIBS)

cli: cli.o torrent_runtime.o
	$(CC) $(CFLAGS) -o bittorrent cli.o torrent_runtime.o $(LDLIBS)

clean:
	rm -rf *~ *.o
