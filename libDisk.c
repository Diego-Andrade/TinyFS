#include <stdio.h>
#include "libDisk.h"
#include "linkedList.h"
#include <sys/types.h>
#include <unistd.h>

#define FORMAT_ISSUE -3
#define OUT_OF_BOUNDS -4

static LList *FDTable;
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
    off_t offset = bNum*BLOCKSIZE;
    fileDescriptor fd;
    Node *entry = getNode(FDTable, disk);

    fd = open(getDiskName(disk));
    if (lseek(fd, offset, SEEK_SET) == -1)    // Sets File offset to offset bytes
    {
        close(fd);
        return -1;
    }
    if (read(fd, block, BLOCKSIZE) < BLOCKSIZE)
    {
        close(fd);
        return BYTES_SMALLER_THAN_BLOCKSIZE;
    }
    close(fd);
    return 0;
}

int writeBlock(int disk, int bNum, void *block)
{
    off_t offset;
    fileDescriptor fd;
    Node *entry;

    entry = getNode(FDTable, disk);
    if (bNum >= entry->numBlocks)
        return OUT_OF_BOUNDS;
    offset = bNum*BLOCKSIZE;
    fd = open(entry->fileName);
    if (lseek(fd, offset, SEEK_SET) == -1)    // Sets File offset to offset bytes
    {
        close(fd);
        return -1;
    }
    if (write(fd, block, BLOCKSIZE) < BLOCKSIZE)
    {
        close(fd);
        return BYTES_SMALLER_THAN_BLOCKSIZE;
    }
    close(fd);
    return 0;
}