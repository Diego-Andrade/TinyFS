#include "libDisk.h"

int openDisk(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE) 
        return BYTES_SMALLER_THAN_BLOCKSIZE;

    int num_blocks = nBytes % BLOCKSIZE;

}

int closeDisk(int disk)
{
    
}

int readBlock(int disk, int bNum, void *block)
{

}

int writeBlock(int disk, int bNum, void *block)
{

}