#include <stdio.h>
#include <string.h>
#include <math.h>

#include "fileTableList.h"
#include "tinyFS.h"
#include "libTinyFS.h"
#include "libDisk.h"
#include "tinyFS_errno.h"

// FS Info
int mountedDisk = 0;
char* mountedDiskName;
unsigned char spBlk[BLOCKSIZE];

//TinyFs Vars
FileTable *openedFilesList;
int counter = 0;
int errorNum = 0;

int tfs_mkfs(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE * 2) return INSUFFICIENT_SPACE;  // Not enough bytes to make a disk

    int d = openDisk(filename, nBytes);

    if (d < 0) RET_ERROR(FAILURE_TO_OPEN);                   // Failed to open disk

    int num_blocks = (int)floor((nBytes*1.0) / (BLOCKSIZE - 2));

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
    bytes2Ptr = (Bytes2_t*)(buffer + 2);     //taking bytes 2-3
    *bytes2Ptr = num_blocks - 2;            //NumBlocks - SuperBlock - Root Inode
    //Empty Block Number
    bytes2Ptr = (Bytes2_t*)(buffer + 4);     //taking bytes 4-5
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
    bytes2Ptr = (Bytes2_t*)(buffer + 2);     //Points to byte 2
    buffer[BLOCKTYPELOC] = FREE;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    for (int i = 2; i < num_blocks; i++)
    {
        if (i+1 != num_blocks)
            *bytes2Ptr = i+1;       //Set pointer to next free block
        else
            *bytes2Ptr = 0;
        
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
    openedFilesList = createFileTable();
    mountedDiskName = diskname;
    if ((errorNum = readBlock(mountedDisk, SUPERBLOCK_BNUM, spBlk)) < 0)
        RET_ERROR(errorNum);

    return 0;
}

int tfs_unmount(void)
{
    purgeTable(openedFilesList);
    openedFilesList = NULL;
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
char *findFile(char *name, int *i, Bytes2_t *currBlock, char *block)
{
    Bytes2_t fileExtent;
    Bytes2_t currBlock_val = (currBlock != NULL) ? *currBlock : RINODE_BNUM;

    while((block)[*i] != '\0')                     //Assumes filename cannot not be NULL
    {
        while(*i + FILE_ENTRY_SIZE - 1 < BLOCKSIZE - 2 && (block)[*i] != '\0')
        {
            if (strcmp(block + *i, name) == 0)           //Filename is found
            {
                if (currBlock)
                    *currBlock = currBlock_val;
                return block + *i;
            }
            *i += FILE_ENTRY_SIZE;
        }
        fileExtent = *((Bytes2_t*)(block + BLOCKSIZE - 2));
        if (fileExtent != 0)      //if FileExtend exists
        {
            currBlock_val = fileExtent;
            if ((errorNum = readBlock(mountedDisk, currBlock_val, block)) < 0)
                return NULL;
            if ((block)[BLOCKTYPELOC] != FILEEXTEND || (block)[MAGICNUMLOC] != MAGICNUMBER)
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
    FileEntry *Entry;
    int i;
    Bytes2_t currBlock;
    int fileExtent;
    char block[BLOCKSIZE];

    if (mountedDisk <= 0)
        RET_ERROR(DISK_NOT_FOUND);
    //Checks if trying to open root directory and if name has too many chars
    if (strcmp(name, "/") == 0 ||
        strlen(name) == 0 || strlen(name) >= MAX_FILENAME_SIZE + 1)
        RET_ERROR(INVALID_NAME);

    //Checks if file is already open
    if ((Entry = findEntry_name(openedFilesList, name)) != NULL)
        return Entry->fd;
    
    //Checks if file is on disk
    currBlock = 1;
    if ((errorNum = readBlock(mountedDisk, currBlock, block)) < 0)
        RET_ERROR(errorNum);
    if (block[BLOCKTYPELOC] != INODE || block[MAGICNUMLOC] != MAGICNUMBER)
        RET_ERROR(FORMAT_ISSUE);
    i = INODE_DATA_START;
    //Searches Root Inode for File and searches fileExtent block if it exists
    int file_inode = findFile(name, &i, &currBlock, block);

    if (file_inode > 0)
    {
        registerEntry(openedFilesList, name, file_inode, 0, counter);
        return counter++;
    }

    // TODO: what is this checking?
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
        if (block[BLOCKTYPELOC] != FILEEXTEND || block[MAGICNUMLOC] != MAGICNUMBER)
            RET_ERROR(FORMAT_ISSUE);
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
    if (block[BLOCKTYPELOC] != FILEEXTEND || block[MAGICNUMLOC] != MAGICNUMBER)
            RET_ERROR(FORMAT_ISSUE);
    block[BLOCKTYPELOC] = INODE;
    strcpy(block + FREE_DATA_START, name);
    *((Bytes2_t*)(block + INODE_SIZE_START)) = 0;     //size
    *((Bytes2_t*)(block + INODE_BLOCKS_START)) = 0;     //data blocks
    *((Bytes2_t*)(block + BLOCKSIZE - 2)) = 0;     //File Extent
    if ((errorNum = writeBlock(mountedDisk, *blockPtr, block)) < 0)
        RET_ERROR(errorNum);

    registerEntry(openedFilesList, name, *blockPtr, 0, counter);
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
    FileEntry *entry;
    Bytes2_t currBlock;
    char *filePtr;
    char block[BLOCKSIZE];
    char *bufferPtr = buffer;
    int i;

    if ((entry = findEntry_fd(openedFilesList, FD)) < 0)
        RET_ERROR(FAILURE_TO_OPEN);
    if ((errorNum = readBlock(mountedDisk, RINODE_BNUM, block)) < 0)
        RET_ERROR(errorNum);
    
    i = INODE_DATA_START;
    currBlock = RINODE_BNUM;
    if ((filePtr = findFile(entry->fileName, &i, &currBlock, block)) == NULL)
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

    int blocksNeeded = ((int)ceil(size / (BLOCKSIZE - 2.0))) - *freeBlocks;
    if (blocksNeeded > *((Bytes2_t*)(spBlk + 2)))
        RET_ERROR(INSUFFICIENT_SPACE);
    
    i = INODE_DATA_START;
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
        if (i + 2 - 1 > BLOCKSIZE - 2)
        {       //Create file extent for inode if file needs more blocks
            blockPtr = (Bytes2_t*)(block + BLOCKSIZE - 2);
            if ((errorNum = writeNextFreeBlock(blockPtr, block)) < 0)
                RET_ERROR(errorNum);
            if ((errorNum = writeBlock(mountedDisk, currBlock, block)) < 0)
                RET_ERROR(errorNum);
            currBlock = *blockPtr;
            if ((errorNum = readBlock(mountedDisk, currBlock, block)) < 0)
                RET_ERROR(errorNum);
            if (block[BLOCKTYPELOC] != FILEEXTEND || block[MAGICNUMLOC] != MAGICNUMBER)
                RET_ERROR(FORMAT_ISSUE);
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
        
        memset(wBlock, 0, BLOCKSIZE);
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
    if (currBlock == bNum)
    {
        *sizePtr = size_l;
        *freeBlocks = freeBlocks_l;
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

/** HELPER: Returns the physical block number of logical file extend block,  
 *  or -1 if failed
**/
Blocknum get_file_extend(char* inode, Blocknum file_ext) {
    Blocknum* fe_list; 

    // File extend found on Inode
    if (file_ext <= INODE_MAX_FE) {
        fe_list = (Blocknum*) (inode + INODE_FE_LIST);          // Get pointer to start of fe list
        return fe_list[file_ext];
    }

    // File extend on inode extend blocks
    int num_of_ext = 1;
    file_ext -= INODE_MAX_FE;                                   // Removed amount of FE checked in inode
    
    num_of_ext += file_ext / (FE_MAX_DATA / sizeof(Blocknum));   // Calculate num of extend blocks from inode
    file_ext = file_ext % (FE_MAX_DATA / sizeof(Blocknum));

    if (file_ext == 0) 
        num_of_ext -= 1;     // File extend found at end of last inode extend

    for ( ; num_of_ext > 0; num_of_ext--) {
        if (readBlock(mountedDisk, inode[INODE_EXTEND], inode) < 0) 
            RET_ERROR(FAILED_TO_READ);
    }
    
    fe_list = (Blocknum*) (inode + FE_DATA);          // Get pointer to start of fe list, using fe block
    return fe_list[file_ext];
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    FileEntry* entry = findEntry_fd(openedFilesList, FD);
    
    // Open file check
    if (entry <= 0) 
        RET_ERROR(INVALID_FD);

    // EOF check
    if (entry->cursor == entry->size)
        RET_ERROR(TFS_EOF);

    char tempBlock[BLOCKSIZE];
    if (readBlock(mountedDisk, entry->inode, &tempBlock) < 0)   // Read inode data
        RET_ERROR(FAILED_TO_READ);

    // Find block that contains requested data
    Blocknum file_ext = entry->cursor / FE_MAX_DATA;    // logical block that fp is at. logblk = cur / data per fe blk
    int loc      = entry->cursor % FE_MAX_DATA;         // Location of fp in file extend

    Blocknum physical_block = get_file_extend(tempBlock, file_ext); 
    if (physical_block <= 0)
        RET_ERROR(FAILED_TO_READ);

    if (readBlock(mountedDisk, physical_block, &tempBlock) < 0)
        RET_ERROR(FAILED_TO_READ);
    
    *buffer = tempBlock[FE_DATA + loc];     // Load buffer with requested Byte
    entry->cursor++;                        // Advance fp once read
    return 0;
}

int tfs_seek(fileDescriptor FD, int offset) {
    FileEntry* entry = findEntry_fd(openedFilesList, FD);
    if (entry <= 0) 
        RET_ERROR(INVALID_FD);

    int new_loc = entry->cursor + offset;
    if (new_loc > entry->size || new_loc < 0)   /** TODO: Potentiallay allow past 0 but trunc to 0 **/
        RET_ERROR(INVALID_FILE_OFFSET);

    entry->cursor = new_loc;
    return 0;
}