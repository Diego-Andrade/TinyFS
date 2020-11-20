#include "libDisk.h"

#define FORMAT_ISSUE -3

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
    int offset = bNum*BLOCKSIZE;
    int i;
    char c;
    fileDescriptor fd;

    fd = open(getDiskName(disk));
    for (i = 0; i < offset; i++)
    {
        if ((c = read(fd, c, 1)) == 0)
            break;
    }

    if (c == 0)    //Read error occured
        return FORMAT_ISSUE;
    if (read(fd, block, BLOCKSIZE) < BLOCKSIZE)
        return BYTES_SMALLER_THAN_BLOCKSIZE;
    return 0;
}

int writeBlock(int disk, int bNum, void *block)
{

}