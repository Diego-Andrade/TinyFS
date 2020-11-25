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
LList *openedFilesList;
int counter = 0;

int tfs_mkfs(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE * 2) return INSUFFICIENT_SPACE;  // Not enough bytes to make a disk

    int d = openDisk(filename, nBytes);

    if (d < 0) return -1;                   // Failed to open disk

    int num_blocks = getDiskSize(d);

    unsigned char buffer[BLOCKSIZE];
    Bytes2_t *bytes2Ptr;

    // Superblock
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = SUPERBLOCK;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    if (num_blocks > 65535)                 //Max Number of blocks
        return -1;    //*error overflow of numblocks counter
    //Number of Free Blocks
    bytes2Ptr = (Bytes2_t)(buffer + 2);     //taking bytes 2-3
    *bytes2Ptr = num_blocks - 2;            //NumBlocks - SuperBlock - Root Inode
    //Empty Block Number
    bytes2Ptr = (Bytes2_t)(buffer + 4);     //taking bytes 4-5
    *bytes2Ptr = 2;

    //buffer[6] = 1;                        //Root Inode
    writeBlock(d, 0, buffer);
    
    //Root Inode
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = INODE;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    buffer[2] = '/';
    buffer[3] = '\0';                       //takes two bytes (2-10)
    //Size
    bytes2Ptr = (Bytes2_t)(buffer + 11);    
    *bytes2Ptr = MAX_FILENAME_SIZE + 1 + 4;  //NameSize(9) + stats(2) + size(2)
    //Inode Extend is NULL at end of file (Bytes BLOCKSIZE - 2 to BLOCKSIZE - 1)
    writeBlock(d, 1, buffer);

    // Empty blocks
    memset(buffer, 0, BLOCKSIZE);
    bytes2Ptr = (Bytes2_t)(buffer + 2);     //Points to byte 2
    buffer[BLOCKTYPELOC] = FREE;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    for (int i = 2; i < num_blocks; i++)
    {
        if (i+1 != num_blocks)
            *bytes2Ptr = i+1;       //Set pointer to next free block
        else
            *bytes2Ptr = NULL;
        
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
    counter = 0;
}

Bytes2_t* Inode_GetBlock(int8_t *byte)
{
    while (*(byte)++ != '\0');
    return (Bytes2_t*)byte;
}

fileDescriptor tfs_openFile(char *name)
{
    Node *Entry;
    int i;
    Bytes2_t currBlock;
    int fileExtent;
    char block[BLOCKSIZE];

    if (mountedDisk < 0)
        return DISK_NOT_FOUND;
    //Checks if trying to open root directory and if name has too many chars
    if (strcmp(name, "/") == 0 ||
        strlen(name) == 0 || strlen(name) >= MAX_FILENAME_SIZE + 1)
        return INVALID_NAME;

    //Checks if file is already open
    if ((Entry = findFileName(openedFilesList, name)) != NULL)
        return Entry->fd;
    
    //Checks if file is on disk
    currBlock = 1;
    if (readBlock(mountedDisk, currBlock, block) < 0)
        return READ_ERROR;
    if (block[0] != INODE || block[1] != MAGICNUMBER)
        return FORMAT_ISSUE;
    i = INODE_DATA_START;
    //Searches Root Inode for File and searches fileExtent block if it exists
    while(block[i] != '\0')                     //Assumes filename cannot not be NULL
    {
        while(i < BLOCKSIZE - 2 && block[i] != '\0')
        {
            if (strcmp(block + i, name) == 0)
            {
                registerEntry(openedFilesList, name, 0, counter);
                return counter++;
            }
            i += FILE_ENTRY_SIZE;
        }
        fileExtent = *((Bytes2_t*)(block + BLOCKSIZE - 2));
        if (fileExtent != 0)      //if FileExtend exists
        {
            currBlock = fileExtent;
            readBlock(mountedDisk, currBlock, block);
            i = FREE_DATA_START;
        }
    }
    //Create a New File
    Bytes2_t *blockPtr;
    if (i + FILE_ENTRY_SIZE < BLOCKSIZE - 2)
    {
        strcpy(block + i, name);
        blockPtr = (Bytes2_t*)(block + i + MAX_FILENAME_SIZE + 1);
        *blockPtr = *((Bytes2_t*)(spBlk + 4));
        if (*blockPtr == 0)
            return NO_MORE_SPACE;
        writeBlock(mountedDisk, *blockPtr, block);
        readBlock(mountedDisk, *blockPtr, block);
        *((Bytes2_t*)(spBlk + 4)) = *((Bytes2_t*)(block + 2));      //Update superblock
        block[BLOCKTYPELOC] = FILEEXTEND;
        block[MAGICNUMLOC] = MAGICNUMBER;
        block[2] = 0;                                               //Set blocknum to 0
        block[3] = 0;
        writeBlock(mountedDisk, *blockPtr, block);
    }
    else        //Needs more space for inode block
    {
        
    }
    
}

int tfs_closeFile(fileDescriptor FD);

int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset);