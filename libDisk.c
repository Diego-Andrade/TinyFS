#include "libDisk.h"

int openDisk(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE) 
        return BYTES_SMALLER_THAN_BLOCKSIZE;

    int num_blocks = nBytes % BLOCKSIZE;

}