#include <stdio.h>

#include "libDisk.h"

// TODO: Don't know where to put this..
static disk_count = 0;

int openDisk(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE) 
        return BYTES_SMALLER_THAN_BLOCKSIZE;

    int disk_num = getDiskNum(filename);

    if (nBytes == 0 && disk_num != -1) 
        return disk_num;

    int num_blocks = nBytes % BLOCKSIZE;
    disk_num = addDisk(filename, num_blocks);

    FILE* file = fopen(filename, "wb+");
    for (int i = 0; i < BLOCKSIZE; i++)
        fprintf(file, "0");
    fclose(file);
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

    close(fd);
    if (c == 0)    //Read error occured
        return FORMAT_ISSUE;
    if (read(fd, block, BLOCKSIZE) < BLOCKSIZE)
        return BYTES_SMALLER_THAN_BLOCKSIZE;
    return 0;
}

int writeBlock(int disk, int bNum, void *block)
{

}