# bittorrent

```
1) Git clone the repository

2) In the terminal, within the repository do: git clone https://github.com/willemt/heapless-bencode.git

3) In the terminal, within the repository do: sudo apt-get install libcurl4-openssl-dev

4) Below is an example of how to run the application:

zdwyn:/mnt/c/wsl/cmsc417/bittorrent$make clean;make cli
rm -rf *~ *.o
gcc -Wall -Iincludes -Wextra -std=gnu99 -ggdb shared.c -c -lcrypto -lm
gcc -Wall -Iincludes -Wextra -std=gnu99 -ggdb -c cli.c -c -lcrypto -lm
gcc -Wall -Iincludes -Wextra -std=gnu99 -ggdb -c ./heapless-bencode/bencode.c
gcc -Wall -Iincludes -Wextra -std=gnu99 -ggdb hash.c -c -lcrypto -lm
gcc -Wall -Iincludes -Wextra -std=gnu99 -ggdb -c torrent_runtime.c -c -lcrypto -lm
gcc -Wall -Iincludes -Wextra -std=gnu99 -ggdb -o bittorrent cli.o torrent_runtime.o shared.o hash.o bencode.o -lcrypto -lm
zdwyn:/mnt/c/wsl/cmsc417/bittorrent$./bittorrent ./test_torrents/ubuntu_singlefile.torrent
Beginning download of torrent:
- name: ubuntu-20.10-desktop-amd64.iso
- size: 2.7 GiB
- pieces: 11223 x 256 KiB
- files:
  | ubuntu-20.10-desktop-amd64.iso (2.7 GiB)
  
zdwyn:/mnt/c/wsl/cmsc417/bittorrent$./bittorrent ./test_torrents/1184-0.txt.torrent
Beginning download of torrent:
- name: installer_x86-x64_60098
- size: 222.9 MiB
- pieces: 892 x 256 KiB
- files:
  | Installer_x86-x64_89006.bat (2.0 KiB)
  | common.dat (2.2 MiB)
  | common.dll (6.7 MiB)
  | install.dat (214.0 MiB)
  ```
