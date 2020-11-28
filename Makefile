CC=gcc
CFLAGS=-Wall -Iincludes -Wextra -std=gnu99 -ggdb
LDLIBS=-lcrypto
OBJS=cli.o torrent_runtime.o bencode.o

all: cli

bencode.o:
	$(CC) $(CFLAGS) -c ./heapless-bencode/bencode.c

torrent_runtime.o: bencode.o
	$(CC) $(CFLAGS) -c torrent_runtime.c -c $(LDLIBS)

cli.o:
	$(CC) $(CFLAGS) -c cli.c -c $(LDLIBS)

cli: cli.o torrent_runtime.o
	$(CC) $(CFLAGS) -o bittorrent $(OBJS) $(LDLIBS)

clean:
	rm -rf *~ *.o
