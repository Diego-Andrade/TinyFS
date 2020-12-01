#include "libTinyFS.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

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
int errorNum = 0;

int tfs_mkfs(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE * 2) return INSUFFICIENT_SPACE;  // Not enough bytes to make a disk

    int d = openDisk(filename, nBytes);

    if (d < 0) RET_ERROR(FAILURE_TO_OPEN);                   // Failed to open disk

    int num_blocks = getDiskSize(d);

    if (num_blocks < 0) RET_ERROR(INSUFFICIENT_SPACE);

    unsigned char buffer[BLOCKSIZE];
    Bytes2_t *bytes2Ptr;

    // Superblock
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = SUPERBLOCK;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    if (num_blocks > 65535)                 //Max Number of blocks
        RET_ERROR(OUT_OF_BOUNDS);    //*error overflow of numblocks counter
    //Number of Free Blocks
    bytes2Ptr = (Bytes2_t)(buffer + 2);     //taking bytes 2-3
    *bytes2Ptr = num_blocks - 2;            //NumBlocks - SuperBlock - Root Inode
    //Empty Block Number
    bytes2Ptr = (Bytes2_t)(buffer + 4);     //taking bytes 4-5
    *bytes2Ptr = 2;

    //buffer[6] = 1;                        //Root Inode
    if ((errorNum = writeBlock(d, SUPERBLOCK_BNUM, buffer)) < 0)
        RET_ERROR(errorNum);
    
    //Root Inode
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = INODE;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    buffer[2] = '/';
    buffer[3] = '\0';                       //filename takes 9 bytes (2-10)

    //Inode Extend is NULL at end of file (Bytes BLOCKSIZE - 2 to BLOCKSIZE - 1)
    if ((errorNum = writeBlock(d, RINODE_BNUM, buffer)) < 0)
        RET_ERROR(errorNum);

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
        
        if ((errorNum = writeBlock(d, i, buffer)) < 0)
            RET_ERROR(errorNum);
    }

    // Possible single inode?
    
    if (closeDisk(d) < 0)
        return -1;

    return 0;
}

int tfs_mount(char *diskname) {
    mountedDisk = openDisk(diskname, 0);
    if (mountedDisk < 0)
        return DISK_NOT_FOUND;
    openedFilesList = createTableList();
    mountedDiskName = diskname;
    if ((errorNum = readBlock(mountedDisk, SUPERBLOCK_BNUM, spBlk)) < 0)
        RET_ERROR(errorNum);

    return 0;
}

int tfs_unmount(void)
{
    purgeTable(openedFilesList);
    counter = 0;
}

//Given the address of the entry (file, bnum) return pointer to start of bnum
Bytes2_t* Inode_GetBlock(int8_t *byte)
{
    while (*(byte)++ != '\0');
    return (Bytes2_t*)byte;
}

//Updates free blocks to be file extent and update the super block
int updateFreeBlock()
{
    char block[BLOCKSIZE];

    int oldBlockNum = *((Bytes2_t*)(spBlk + 4));
    if ((errorNum = readBlock(mountedDisk, oldBlockNum, block)) < 0)
        return errorNum;

    //Update super block
    *((Bytes2_t*)(spBlk + 4)) = *((Bytes2_t*)(block + 2));
    *((Bytes2_t*)(spBlk + 2)) -= 1;
    if ((errorNum = writeBlock(mountedDisk, SUPERBLOCK_BNUM, spBlk)) < 0)
        return errorNum;

    block[BLOCKTYPELOC] = FILEEXTEND;
    block[MAGICNUMLOC] = MAGICNUMBER;
    block[2] = 0;                                               //Set blocknum to 0
    block[3] = 0;
    if ((errorNum = writeBlock(mountedDisk, oldBlockNum, block)) < 0)
        return errorNum;
    return 0;
}

//Write next free block into a given block at destination (dest)
//And calls updateFreeBlock() to update both the super block and free block
int writeNextFreeBlock(Bytes2_t* dest, char *block)
{
    *dest = *((Bytes2_t*)(spBlk + 4));
    if (*dest == 0)
        return INSUFFICIENT_SPACE;
    if (updateFreeBlock() < 0)
        return errorNum;
    return 0;
}

//Helper function to find the location of a file name. Returns Null on read error and if not found
// i = starting location, currBlock is a return variable to tell where it ended up and can be null, and block is a pointer to an allocated buffer
//currBlock will be inaccurate if issue from read thus errorNum < 0
char *findFile(char *name, int *i, Bytes2_t *currBlock, char **block)
{
    Bytes2_t fileExtent;
    Bytes2_t currBlock_val = (currBlock != NULL) ? *currBlock : RINODE_BNUM;

    while((*block)[*i] != '\0')                     //Assumes filename cannot not be NULL
    {
        while(*i < BLOCKSIZE - 2 && (*block)[*i] != '\0')
        {
            if (strcmp(*block + *i, name) == 0)           //Filename is found
            {
                if (currBlock)
                    *currBlock = currBlock_val;
                return *block + *i;
            }
            *i += FILE_ENTRY_SIZE;
        }
        fileExtent = *((Bytes2_t*)(*block + BLOCKSIZE - 2));
        if (fileExtent != 0)      //if FileExtend exists
        {
            currBlock_val = fileExtent;
            if ((errorNum = readBlock(mountedDisk, currBlock_val, *block)) < 0)
                return NULL;
            if ((*block)[BLOCKTYPELOC] != FILEEXTEND || (*block)[MAGICNUMLOC] != MAGICNUMBER)
            {
                errorNum = FORMAT_ISSUE;
                return NULL;
            }
            *i = FREE_DATA_START;
        }
    }
    if (currBlock)
        *currBlock = currBlock_val;
    return NULL;
}

fileDescriptor tfs_openFile(char *name)
{
    Node *Entry;
    int i;
    Bytes2_t currBlock;
    int fileExtent;
    char block[BLOCKSIZE];

    if (mountedDisk < 0)
        RET_ERROR(DISK_NOT_FOUND);
    //Checks if trying to open root directory and if name has too many chars
    if (strcmp(name, "/") == 0 ||
        strlen(name) == 0 || strlen(name) >= MAX_FILENAME_SIZE + 1)
        RET_ERROR(INVALID_NAME);

    //Checks if file is already open
    if ((Entry = findFileName(openedFilesList, name)) != NULL)
        return Entry->fd;
    
    //Checks if file is on disk
    currBlock = 1;
    if ((errorNum = readBlock(mountedDisk, currBlock, block)) < 0)
        RET_ERROR(errorNum);
    if (block[BLOCKTYPELOC] != INODE || block[MAGICNUMLOC] != MAGICNUMBER)
        RET_ERROR(FORMAT_ISSUE);
    i = INODE_DATA_START;
    //Searches Root Inode for File and searches fileExtent block if it exists
    if (findFile(Entry->fileName, &i, &currBlock, &block) != NULL)
    {
        registerEntry(openedFilesList, name, 0, counter);
        return counter++;
    }
    if (errorNum < 0)
        RET_ERROR(errorNum);
    
    //Create a New File
    Bytes2_t *blockPtr = (Bytes2_t*)(block + BLOCKSIZE - 2);
    if (i + FILE_ENTRY_SIZE >= BLOCKSIZE - 2)
    {
        if ((errorNum = writeNextFreeBlock(blockPtr, block)) < 0)
            RET_ERROR(errorNum);
        if ((errorNum = writeBlock(mountedDisk, currBlock, block)) < 0)
            RET_ERROR(errorNum);

        currBlock = *blockPtr;
        if ((errorNum = readBlock(mountedDisk, currBlock, block)) < 0)
            RET_ERROR(errorNum);
        if (block[BLOCKTYPELOC] != FREE || block[MAGICNUMLOC] != MAGICNUMBER)
            RET_ERROR(FORMAT_ISSUE);
        block[BLOCKTYPELOC] = FILEEXTEND;
        i = FREE_DATA_START;
    }
    blockPtr = (Bytes2_t*)(block + i + MAX_FILENAME_SIZE + 1);
    strcpy(block + i, name);
    if ((errorNum = writeNextFreeBlock(blockPtr, block)) < 0)
        RET_ERROR(errorNum);
    if ((errorNum = writeBlock(mountedDisk, currBlock, block)) < 0)
        RET_ERROR(errorNum);

    //Update free block to inode block with the name of file
    if ((errorNum = readBlock(mountedDisk, *blockPtr, block)) < 0)
        RET_ERROR(errorNum);
    if (block[BLOCKTYPELOC] != FREE || block[MAGICNUMLOC] != MAGICNUMBER)
            RET_ERROR(FORMAT_ISSUE);
    block[BLOCKTYPELOC] = INODE;
    strcpy(block + FREE_DATA_START, name);
    *((Bytes2_t*)(block + INODE_SIZE_START)) = 0;     //size
    *((Bytes2_t*)(block + INODE_BLOCKS_START)) = 0;     //data blocks
    *((Bytes2_t*)(block + BLOCKSIZE - 2)) = 0;     //File Extent
    if ((errorNum = writeBlock(mountedDisk, *blockPtr, block)) < 0)
        RET_ERROR(errorNum);

    registerEntry(openedFilesList, name, 0, counter);
    return counter++;
}

int tfs_closeFile(fileDescriptor FD)
{
    if (removeEntry(openedFilesList, FD) < 0)
        return EMPTY_LIST;
    return 0;
}

int tfs_writeFile(fileDescriptor FD,char *buffer, int size)
{
    Node *entry;
    Bytes2_t currBlock;
    char *filePtr;
    char block[BLOCKSIZE];
    char *bufferPtr = buffer;

    if ((entry = findEntry_fd(openedFilesList, FD)) < 0)
        RET_ERROR(FAILURE_TO_OPEN);
    if ((errorNum = readBlock(mountedDisk, RINODE_BNUM, block)) < 0)
        RET_ERROR(errorNum);
    
    int i = INODE_DATA_START;
    currBlock = RINODE_BNUM;
    if ((filePtr = findFile(entry->fileName, &i, &currBlock, &block)) == NULL)
    {
        if (errorNum < 0)
            RET_ERROR(errorNum);
        RET_ERROR(FILE_NOT_FOUND);
    }

    Bytes2_t bNum = *((Bytes2_t*)(filePtr + MAX_FILENAME_SIZE + 1));
    if ((errorNum = readBlock(mountedDisk, bNum, block)) < 0)
        RET_ERROR(errorNum);

    Bytes2_t *sizePtr = (Bytes2_t*)(block + INODE_SIZE_START);
    Bytes2_t *freeBlocks = (Bytes2_t*)(block + INODE_BLOCKS_START);

    int blocksNeeded = ((int)ceil(*sizePtr / (BLOCKSIZE - 2.0))) - *freeBlocks;
    if (blocksNeeded > *((Bytes2_t*)(spBlk + 2)))
        RET_ERROR(INSUFFICIENT_SPACE);
    
    int i = INODE_DATA_START;
    Bytes2_t freeBlocks_l = *freeBlocks;
    Bytes2_t blockCounter = freeBlocks_l;
    Bytes2_t size_l = size;
    currBlock = bNum;
    Bytes2_t *blockPtr;
    char wBlock[BLOCKSIZE];
    
    wBlock[BLOCKTYPELOC] = FILEEXTEND;
    wBlock[MAGICNUMBER] = MAGICNUMBER;

    entry->cursor = 0;  //Reset cursor
    while (size > 0)
    {
        if (i + block + 2 > BLOCKSIZE - 2)
        {       //Create file extent for inode if file needs more blocks
            blockPtr = (Bytes2_t*)(block + BLOCKSIZE - 2);
            if ((errorNum = writeNextFreeBlock(blockPtr, block)) < 0)
                RET_ERROR(errorNum);
            if ((errorNum = writeBlock(mountedDisk, currBlock, block)) < 0)
                RET_ERROR(errorNum);
            currBlock = *blockPtr;
            if ((errorNum = readBlock(mountedDisk, currBlock, block)) < 0)
                RET_ERROR(errorNum);
            if (block[BLOCKTYPELOC] != FREE || block[MAGICNUMLOC] != MAGICNUMBER)
                RET_ERROR(FORMAT_ISSUE);
            block[BLOCKTYPELOC] = FILEEXTEND;
            i = FREE_DATA_START;
        }

        blockPtr = (Bytes2_t*)(block + i);
        if (blockCounter <= 0)
        {   //get free block
            if ((errorNum = writeNextFreeBlock(blockPtr, block)) < 0)
                RET_ERROR(errorNum);
            freeBlocks_l += 1;
        }
        else
            blockCounter -= 1;
        

        if (size < BLOCKSIZE - 2)
        {
            memcpy(wBlock + FREE_DATA_START, bufferPtr, size);
            bufferPtr += size;
            size = 0;
        }
        else
        {
            memcpy(wBlock + FREE_DATA_START, bufferPtr, BLOCKSIZE - 2);
            bufferPtr += BLOCKSIZE - 2;
            size -= BLOCKSIZE - 2;
        }
        if ((errorNum = writeBlock(mountedDisk, *blockPtr, wBlock)) < 0)        //Write to data block
            RET_ERROR(errorNum);
        i += 2;                                                                 //Size of blockNum
    }

    if ((errorNum = writeBlock(mountedDisk, currBlock, block)) < 0)        //Write the current block to save new data blocks
        RET_ERROR(errorNum);
    if (currBlock != bNum)
    {
        if ((errorNum = readBlock(mountedDisk, bNum, block)) < 0)
            RET_ERROR(errorNum);
        *sizePtr = size_l;
        *freeBlocks = freeBlocks_l;
        if ((errorNum = writeBlock(mountedDisk, bNum, block)) < 0)        //Update File status data
            RET_ERROR(errorNum);
    }
}

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset);