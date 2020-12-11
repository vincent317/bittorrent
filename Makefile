CC=gcc
CFLAGS=-Wall -Iincludes -Wextra -std=gnu99 -ggdb
LDLIBS=-lcrypto -lm -lcurl -pthread
OBJS=shared.o file_assembler.o cli.o torrent_runtime.o peer_manager.o bencode.o hash.o piece_manager.o piece_manager_data.o upload_download_manager.o

all: cli

shared.o:
	$(CC) $(CFLAGS) shared.c -c $(LDLIBS)

hash.o:
	$(CC) $(CFLAGS) hash.c -c $(LDLIBS)

file_assembler.o:
	$(CC) $(CFLAGS) file_assembler.c -c $(LDLIBS)

upload_download_manager.o:
	$(CC) $(CFLAGS) upload_download_manager.c -c $(LDLIBS)

piece_manager_data.o:
	$(CC) $(CFLAGS) piece_manager_data.c -c $(LDLIBS)

piece_manager.o: shared.o piece_manager_data.o upload_download_manager.o torrent_runtime.o file_assembler.o
	$(CC) $(CFLAGS) piece_manager.c -c $(LDLIBS)

peer_manager.o: shared.o upload_download_manager.o
	$(CC) $(CFLAGS) peer_manager.c -c $(LDLIBS)

bencode.o:
	$(CC) $(CFLAGS) -c ./heapless-bencode/bencode.c

torrent_runtime.o: hash.o shared.o bencode.o peer_manager.o piece_manager.o
	$(CC) $(CFLAGS) -c torrent_runtime.c -c $(LDLIBS)

cli.o:
	$(CC) $(CFLAGS) -c cli.c -c $(LDLIBS)

cli: shared.o cli.o torrent_runtime.o
	$(CC) $(CFLAGS) -o bittorrent $(OBJS) $(LDLIBS)

clean:
	rm -rf *~ *.o

