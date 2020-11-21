#include <stdio.h>
#include <stdlib.h>

#include "libDisk.h"
#include "linkedList.h"

static LList *mountTable;
static int disk_count = 0;

int openDisk(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE) 
        return BYTES_SMALLER_THAN_BLOCKSIZE;

    if (mountTable == NULL)
        mountTable = createLinkedList();
        
    int disk_num = getDiskNum(mountTable, filename);

    if (nBytes == 0 && disk_num > 0) 
        return disk_num;

    int num_blocks = nBytes / BLOCKSIZE;
    disk_num = ++disk_count;
    
    registerDisk(mountTable, disk_num, filename, num_blocks);

    FILE* file = fopen(filename, "wb+");
    for (int i = 0; i < num_blocks * BLOCKSIZE; i++)
        fprintf(file, "0");
    fclose(file);

    return disk_num;
}

int closeDisk(int disk) {
    if (removeNode(mountTable, disk) < 0)
        return -1;
    if (mountTable->numEntries == 0)
        purgeList(mountTable);
    return 0;
}

int readBlock(int disk, int bNum, void *block)
{
    off_t offset = bNum*BLOCKSIZE;
    FILE* file;
    Node *entry = getNode(mountTable, disk);

    file = fopen(entry->fileName, "rb");
    if (file == NULL)
        return;
    if (fseek(file, offset, SEEK_SET) == -1)    // Sets File offset to offset bytes
    {
        close(file);
        return -1;
    }
    if (fread(block, BLOCKSIZE, 1, file) < 1)
    {
        close(file);
        return BYTES_SMALLER_THAN_BLOCKSIZE;
    }
    close(file);
    return 0;
}

int writeBlock(int disk, int bNum, void *block)
{
    off_t offset;
    FILE* file;
    Node *entry;

    entry = getNode(mountTable, disk);
    if (bNum >= entry->numBlocks)
        return OUT_OF_BOUNDS;
    offset = bNum*BLOCKSIZE;
    file = fopen(entry->fileName, "rb");
    if (file == NULL)
        return -1;
    file = fopen(entry->fileName, "wb");
    if (fseek(file, offset, SEEK_SET) == -1)    // Sets File offset to offset bytes
    {
        close(file);
        return -1;
    }
    if (fwrite(block, BLOCKSIZE, 1, file) < 1)
    {
        close(file);
        return BYTES_SMALLER_THAN_BLOCKSIZE;
    }
    close(file);
    return 0;
}