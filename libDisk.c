#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "tinyFS.h"
#include "libDisk.h"
#include "linkedList.h"
#include "tinyFS_errno.h"

static LList *mountTable;
static int disk_count = 0;

int openDisk(char *filename, int nBytes) {
    int disk_num = 0;    
    int num_blocks = nBytes / BLOCKSIZE;

    if (mountTable == NULL)
        mountTable = createLinkedList();
    disk_num = getDiskNum(mountTable, filename);
    
    if (nBytes == 0 && disk_num > 0) 
        return disk_num;

    FILE* file = fopen(filename, "rb+");
    if (file == NULL || nBytes > 0) {
        if (nBytes < BLOCKSIZE)
            return BYTES_SMALLER_THAN_BLOCKSIZE;

        FILE* file = fopen(filename, "wb");
        fseek(file, num_blocks * BLOCKSIZE - 1, SEEK_SET);
        fwrite("\0", 1, 1, file);
        fclose(file);
    }

    struct stat sb;
    stat(filename, &sb);
    
    if (disk_num > 0)
        removeNode(mountTable, disk_num);
    else
        disk_num = ++disk_count;
    registerDisk(mountTable, disk_num, filename, (int) (sb.st_size / BLOCKSIZE));    

    return disk_num;
}

int closeDisk(int disk) {
    if (removeNode(mountTable, disk) < 0)
        return -1;
    
    if (mountTable->numEntries == 0)
    {
        disk_count = 0;
        purgeList(mountTable);
        mountTable = NULL;
    }
    return 0;
}

int readBlock(int disk, int bNum, void *block)
{
    off_t offset = bNum*BLOCKSIZE;
    FILE* file;
    Node *entry = getNode(mountTable, disk);

    file = fopen(entry->fileName, "rb");
    if (file == NULL)
        return FILE_NULL;
    if (fseek(file, offset, SEEK_SET) == -1)    // Sets File offset to offset bytes
    {
        fclose(file);
        return -1;
    }
    if (fread(block, BLOCKSIZE, 1, file) < 1)
    {
        fclose(file);
        return BYTES_SMALLER_THAN_BLOCKSIZE;
    }
    fclose(file);
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
    file = fopen(entry->fileName, "rb+");
    if (file == NULL)
        return FILE_NULL;

    if (fseek(file, offset, SEEK_SET) == -1)    // Sets File offset to offset bytes
    {
        fclose(file);
        return BLOCK_WRITE_FAILED;
    }
    if (fwrite(block, BLOCKSIZE, 1, file) < 1)
    {
        fclose(file);
        return BYTES_SMALLER_THAN_BLOCKSIZE;
    }
    fclose(file);
    return 0;
}

// Returns the number of blocks in a disk
int getDiskSize(int disk) {
    Node *entry = getNode(mountTable, disk);

    if (entry) 
        return entry->numBlocks;

    return -1;
}