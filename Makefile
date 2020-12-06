CC=gcc
CFLAGS=-Wall -Iincludes -Wextra -std=gnu99 -ggdb
LDLIBS=-lcrypto -lm -lcurl
OBJS=cli.o torrent_runtime.o shared.o hash.o bencode.o upload_download_manager.o

all: cli

upload_download_manager.o:
	$(CC) $(CFLAGS) upload_download_manager.c -c $(LDLIBS)

piece_manager.o:
	$(CC) $(CFLAGS) piece_manager.c -c $(LDLIBS)

piece_manager_data.o:
	$(CC) $(CFLAGS) piece_manager_data.c -c $(LDLIBS)

peer_manager.o:
	$(CC) $(CFLAGS) peer_manager.c -c $(LDLIBS)

shared.o:
	$(CC) $(CFLAGS) shared.c -c $(LDLIBS)

hash.o:
	$(CC) $(CFLAGS) hash.c -c $(LDLIBS)

bencode.o:
	$(CC) $(CFLAGS) -c ./heapless-bencode/bencode.c

torrent_runtime.o: bencode.o hash.o peer_manager.o
	$(CC) $(CFLAGS) -c torrent_runtime.c -c $(LDLIBS)

cli.o:
	$(CC) $(CFLAGS) -c cli.c -c $(LDLIBS)

cli: shared.o cli.o torrent_runtime.o piece_manager.o piece_manager_data.o upload_download_manager.o
	$(CC) $(CFLAGS) peer_manager.o -o bittorrent $(OBJS) $(LDLIBS)

clean:
	rm -rf *~ *.o
