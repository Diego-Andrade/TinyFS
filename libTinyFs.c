#include "libTinyFS.h"

#include <stdio.h>
#include <string.h>

#include "fileTableList.h"
#include "tinyFS.h"
#include "libDisk.h"
#include "tinyFS_errno.h"

// FS Info
int mountedDisk = 0;
char* mountedDiskName;
unsigned char spBlk[BLOCKSIZE];

//TinyFs Vars
LList openedFilesList;
int counter = 0;

int tfs_mkfs(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE * 2) return -1;  // Not enough bytes to make a disk

    int d = openDisk(filename, nBytes);

    if (d < 0) return -1;                   // Failed to open disk

    int num_blocks = getDiskSize(d);

    unsigned char buffer[BLOCKSIZE];
    int32_t *freeBlockPt;
    int16_t *numFreeBlocks;

    // Superblock
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = SUPERBLOCK;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    if (num_blocks > 65535)                 //Max Number of blocks
        return -1;    //*error overflow of numblocks counter
    numFreeBlocks = buffer + 2;
    *numFreeBlocks = num_blocks;            //numFreeBlocks taking bytes 2-3
    freeBlockPt = buffer + 4;               //Empty block ptr taking bytes 4-7
    *freeBlockPt = BLOCKSIZE;
    buffer[8] = NULL                        //End of Inode pointer
    writeBlock(d, 0, buffer);
    
    // Empty blocks
    freeBlockPt = buffer + 2;             //Points to byte 2
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = FREE;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    for (int i = 1; i < num_blocks; i++)
    {
        if (i+1 != num_blocks)
            *freeBlockPt = (BLOCKSIZE * i) + 2; //Set pointer to next free block
        else
            *freeBlockPt = NULL;
        
        writeBlock(d, i, buffer);
    }

    // Possible single inode?
    
    if (closeDisk(d) < 0)
        return -1;

    return 0;
}

int tfs_mount(char *diskname) {
    mountedDisk = openDisk(diskname, 0);
    if (mountedDisk < 0)
        return DISK_NOT_FOUND_TO_MOUNT;
    
    openedFilesList = createTableList();
    mountedDiskName = diskname;
    readBlock(mountedDisk, 0, spBlk);

    return 0;
}

int tfs_unmount(void)
{
    purgeTable(openedFilesList);
}

fileDescriptor tfs_openFile(char *name)
{
    int fd;

    //Checks if file is already open
    if ((fd = findFileName(openedFilesList, name)) >= 0)
        return fd;
    
    //Checks if file is on disk
    
}

int tfs_closeFile(fileDescriptor FD);

int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset);