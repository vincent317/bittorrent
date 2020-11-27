CC=gcc
CFLAGS=-Wall -Iincludes -Wextra -std=gnu99 -ggdb
LDLIBS=-lcrypto
OBJS=cli.o torrent_runtime.o

all: cli

torrent_runtime.o:
	$(CC) $(CFLAGS) torrent_runtime.c ./heapless-bencode/bencode.c -c $(LDLIBS)

cli.o:
	$(CC) $(CFLAGS) cli.c cli.h shared.h -c $(LDLIBS)

cli: cli.o torrent_runtime.o
	$(CC) $(CFLAGS) -o bittorrent $(OBJS) $(LDLIBS)

clean:
	rm -rf *~ *.o
