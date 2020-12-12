
#ifndef FILE_ASSEMBLER_H_INCLUDED
#define FILE_ASSEMBLER_H_INCLUDED

#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "shared.h"
#include "torrent_runtime.h"
#include "peer_manager.h"
#include "upload_download_manager.h"

// Assemble the piece into files
void file_assembler_begin(Torrent * torrent);

// Read amount of byte from readFile and write it to writeFile starting at index beginIndex
void file_assembler_read_then_write(FILE * readFile, FILE * writeFile, uint64_t beginIndex, uint64_t amount);

#endif
