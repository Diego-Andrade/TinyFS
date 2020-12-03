#include <stdio.h>
#include <string.h>

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

    int num_blocks = nBytes / BLOCKSIZE;                    // Truncate to nearest block size

    if (num_blocks < 0) RET_ERROR(INSUFFICIENT_SPACE);

    unsigned char buffer[BLOCKSIZE];
    Bytes2_t *bytes2Ptr;

    // Superblock
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = SUPERBLOCK;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    if (num_blocks > MAX_NUMBLOCKS)                 //Max Number of blocks
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
    buffer[INODE_SIZE_START] = 0;
    buffer[INODE_SIZE_START + 1] = 0;
    buffer[INODE_NAME_START] = '/';
    buffer[INODE_NAME_START + 1] = '\0';                       //filename takes 9 bytes (4-12)

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
//must read root inode before calling function
char *findFile(char *name, int *currBlock, char *block)
{
    Bytes2_t *fileExtent = (Bytes2_t*)(block + INODE_EXTEND);
    Bytes2_t *num_files = (Bytes2_t*) (block + INODE_SIZE_START);
    Bytes2_t filesToFind;
    int i = INODE_DATA_START;

    if (*currBlock != RINODE_BNUM)
    {
        if ((errorNum = readBlock(mountedDisk, RINODE_BNUM, block)) < 0)
            return NULL;
        *currBlock = RINODE_BNUM;
    }

    filesToFind = *num_files;

    while(filesToFind > 0)                     //Assumes filename cannot not be NULL
    {
        if (block[i] != '\0')       //Empty Entry
        {
            if (strcmp(block + i, name) == 0)       //Filename is found
                return block + i;
            filesToFind -= 1;                       //Decrements counter if file not found
        }
        if (filesToFind > 0 && i + FILE_ENTRY_SIZE - 1 >= BLOCKSIZE - 2)   //If next iteration is out of bounds
        {
            if (*fileExtent == 0)
            {
                errorNum = FORMAT_ISSUE;
                return NULL;
            }
            *currBlock = *fileExtent;
            if ((errorNum = readBlock(mountedDisk, *fileExtent, block)) < 0)
                return NULL;
            if ((block)[BLOCKTYPELOC] != FILEEXTEND || (block)[MAGICNUMLOC] != MAGICNUMBER)
            {
                errorNum = FORMAT_ISSUE;
                return NULL;
            }
            i = FE_DATA + sizeof(Blocknum);
        }
        i += FILE_ENTRY_SIZE;
    }
    return NULL;
}

int findOpenEntry(int *currBlock, char *block)
{
    int i = INODE_DATA_START;
    while (i < BLOCKSIZE - 2 || *((Blocknum*)(block + INODE_EXTEND)) != 0)
    {
        if (i >= BLOCKSIZE - 2) //Go to file extend block if this is true and still in loop
        {
            if (*((Blocknum*)(block + INODE_EXTEND)) == 0)
            {
                errorNum = FORMAT_ISSUE;
                return -1;
            }
            *currBlock = *((Blocknum*)(block + INODE_EXTEND));
            if ((errorNum = readBlock(mountedDisk, *currBlock, block)) < 0)
                return -1;
            i = FE_DATA + sizeof(Blocknum);
        }
        if (block[i] == '\0')
            return i;
        i += FILE_ENTRY_SIZE;
    }
    return -1;
}

fileDescriptor tfs_openFile(char *name)
{
    FileEntry *Entry;
    int newFileLocation;
    Bytes2_t currBlock;
    int fileExtent;
    char *fileLocation;
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
    currBlock = RINODE_BNUM;
    if ((errorNum = readBlock(mountedDisk, currBlock, block)) < 0)
        RET_ERROR(errorNum);
    if (block[BLOCKTYPELOC] != INODE || block[MAGICNUMLOC] != MAGICNUMBER)
        RET_ERROR(FORMAT_ISSUE);

    //Searches Root Inode for File and searches fileExtent block if it exists
    if ((fileLocation = findFile(name, &currBlock, block)) != NULL)
    {
        Blocknum bnum = *((Blocknum*)(fileLocation + MAX_FILENAME_SIZE + 1));
        if ((errorNum = readBlock(mountedDisk, bnum, block)) < 0)
            RET_ERROR(errorNum);
        registerEntry(openedFilesList, name, bnum, *((Blocknum*)(block + INODE_SIZE_START)), counter);
        return counter++;
    }
    if (errorNum < 0)           //Check if an error occured during FindFile
        RET_ERROR(errorNum);
    
    if (currBlock != RINODE_BNUM)
    {
        if ((errorNum = readBlock(mountedDisk, RINODE_BNUM, block)) < 0)
            return NULL;
        currBlock = RINODE_BNUM;
    }
    
    if ((newFileLocation = findOpenEntry(&currBlock, block)) < 0)
    {
        if (errorNum < 0)
            RET_ERROR(errorNum);
        RET_ERROR(INSUFFICIENT_SPACE);
    }

    //Create a New File
    Bytes2_t *blockPtr = (Bytes2_t*)(block + BLOCKSIZE - 2);
    if (newFileLocation + FILE_ENTRY_SIZE >= FE_MAX_DATA)   //Get Free block and place file there
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
        newFileLocation = FREE_DATA_START + sizeof(Blocknum);
    }
    blockPtr = (Bytes2_t*)(block + newFileLocation + MAX_FILENAME_SIZE + 1);    //Points to new entry
    *((Bytes2_t*)(block + INODE_SIZE_START)) += 1;              //updates file number in inode
    strcpy(block + newFileLocation, name);                      //Writes name
    if ((errorNum = writeNextFreeBlock(blockPtr, block)) < 0)   //Gets blocknum for the file
        RET_ERROR(errorNum);
    if ((errorNum = writeBlock(mountedDisk, currBlock, block)) < 0)
        RET_ERROR(errorNum);

    currBlock = *blockPtr;
    //Update free block to inode block with the name of file
    if ((errorNum = readBlock(mountedDisk, currBlock, block)) < 0)
        RET_ERROR(errorNum);
    if (block[BLOCKTYPELOC] != FILEEXTEND || block[MAGICNUMLOC] != MAGICNUMBER)
            RET_ERROR(FORMAT_ISSUE);
    block[BLOCKTYPELOC] = INODE;
    strcpy(block + INODE_NAME_START, name);           //Writes Name
    *((Bytes2_t*)(block + INODE_SIZE_START)) = 0;     //Resets Size
    *((Bytes2_t*)(block + INODE_BLOCKS_START)) = 0;   //Resets Number of Blocks
    *((Bytes2_t*)(block + BLOCKSIZE - 2)) = 0;        //Resets File Extent

    if ((errorNum = writeBlock(mountedDisk, currBlock, block)) < 0)
        RET_ERROR(errorNum);

    registerEntry(openedFilesList, name, currBlock, 0, counter);
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

    if ((entry = findEntry_fd(openedFilesList, FD)) == NULL)
        RET_ERROR(FAILURE_TO_OPEN);
    if ((errorNum = readBlock(mountedDisk, RINODE_BNUM, block)) < 0)
        RET_ERROR(errorNum);
    
    if ((filePtr = findFile(entry->fileName, &currBlock, block)) == NULL)
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

    int blocksNeeded = size / FE_MAX_DATA;
    if (size % FE_MAX_DATA > 0)
        blocksNeeded += 1;
    blocksNeeded -= *freeBlocks;
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
    wBlock[MAGICNUMLOC] = MAGICNUMBER;

    entry->cursor = 0;  //Reset cursor
    entry->size = 0;
    while (size > 0)
    {
        blockPtr = (Bytes2_t*)(block + i);
        if (blockCounter <= 0)
        {   //get free block
            if ((errorNum = writeNextFreeBlock(blockPtr, block)) < 0)
                RET_ERROR(errorNum);
            freeBlocks_l += 1;
        }
        else
            blockCounter -= 1;
        
        memset(wBlock + FREE_DATA_START, 0, BLOCKSIZE - FREE_DATA_START);         // Delete only data portion and not format
        if (size < FE_MAX_DATA)
        {
            memcpy(wBlock + FREE_DATA_START, bufferPtr, size);
            bufferPtr += size;
            entry->size += size;
            size = 0;
        }
        else
        {
            memcpy(wBlock + FREE_DATA_START, bufferPtr, FE_MAX_DATA);
            bufferPtr += FE_MAX_DATA;
            entry->size += FE_MAX_DATA;
            size -= FE_MAX_DATA;
        }
        if ((errorNum = writeBlock(mountedDisk, *blockPtr, wBlock)) < 0)        //Write to data block
            RET_ERROR(errorNum);
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

/** HELPER: Get the physical block number of logical file_ext block  
 *  Return: physical block number or error code
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
    
    num_of_ext += file_ext / (FE_MAX_DATA / sizeof(Blocknum));  // Calculate num of extend blocks from inode
    file_ext = file_ext % (FE_MAX_DATA / sizeof(Blocknum));

    // Is file extend at end of last inode extend?
    if (file_ext == 0) 
        num_of_ext -= 1;     

    for ( ; num_of_ext > 0; num_of_ext--) 
        if (readBlock(mountedDisk, inode[INODE_EXTEND], inode) < 0) 
            RET_ERROR(FAILED_TO_READ);
    
    fe_list = (Blocknum*) (inode + FE_DATA);                    // Get pointer to start of fe list, using fe block
    return fe_list[file_ext];
}

/** HELPER: Erases a give block and adds to free block list **/
int free_block(Blocknum bNum) {
    char empty_block[BLOCKSIZE];
    memset(empty_block, 0, BLOCKSIZE);
    empty_block[BLOCKTYPELOC] = FREE_TYPE;
    empty_block[MAGICNUMLOC] = MAGICNUMBER;
    empty_block[FREE_DATA_START] = spBlk[SUPER_FREE_LIST];      // Adding link 
    if (writeBlock(mountedDisk, bNum, empty_block) < 0)
        RET_ERROR(BLOCK_WRITE_FAILED);

    // Update superblock
    spBlk[SUPER_FREE_LIST] = bNum;
    spBlk[SUPER_FREE_COUNT] += 1;
    if (writeBlock(mountedDisk, SUPERBLOCK_BNUM, spBlk) < 0)
        RET_ERROR(BLOCK_WRITE_FAILED);

    return 0;
}

/** HELPER: Removes file entry from root directory **/
int removed_file_entry_from_directory(char* name) {
    char temp[BLOCKSIZE];

    Blocknum lastBlk = -1;              // Used to removed inode extend
    Blocknum currBlk = RINODE_BNUM;     // Start search in root inode
    char* offset;                       // Offset inside temp
    
    // Find entry in root inode or extend blocks
    while (currBlk != 0) {
        if (readBlock(mountedDisk, currBlk, temp) < 0) 
            RET_ERROR(FAILED_TO_READ);

        offset = temp + INODE_NAME_START;                                       // Start a list of filenames
        for( ; offset != temp + INODE_EXTEND ; offset += FILE_ENTRY_SIZE) {     // Go through all until reach extend pointer
            if (strncmp(name, offset, MAX_FILENAME_SIZE + 1) == 0) {             // Compare first MAX_FILENAME_SIZE + 1 (null terminator)
                goto Erase;
            }
        }

        lastBlk = currBlk;
        currBlk = temp[INODE_EXTEND];
    }

    
    // Erase data
    Erase: 
    if (currBlk == 0)
        RET_ERROR(FILE_NOT_FOUND);
    
    Bytes2_t* size = (Bytes2_t*) (temp+INODE_SIZE_START);
    *size -= 1;
    
    // Removed inode extend
    if (size == 0 && lastBlk != -1) {      
        Blocknum next_extend = temp[INODE_EXTEND];          // Next link
        free_block(currBlk);
        
        if (readBlock(mountedDisk, lastBlk, temp) < 0)      // Prev link
            RET_ERROR(FAILED_TO_READ);

        temp[INODE_EXTEND] = next_extend;                   // Removing extend by linking past
        if (writeBlock(mountedDisk, lastBlk, temp) < 0) 
            RET_ERROR(BLOCK_WRITE_FAILED);

    } else {
        memset(offset, 0, FILE_ENTRY_SIZE);
        if (writeBlock(mountedDisk, currBlk, temp) < 0) 
            RET_ERROR(BLOCK_WRITE_FAILED);
    }

    return 0;
}

int tfs_deleteFile(fileDescriptor FD) {
    FileEntry* entry = findEntry_fd(openedFilesList, FD);
    
    // Open file check
    if (entry <= 0) 
        RET_ERROR(INVALID_FD);

    char inode[BLOCKSIZE];
    if (readBlock(mountedDisk, entry->inode, inode) < 0) 
            RET_ERROR(FAILED_TO_READ);

    // Erase data blocks
    int total_data_blks = entry->size / FE_MAX_DATA;        // Number of File Extend data blocks to free
    if (entry->size % FE_MAX_DATA > 0)
        total_data_blks += 1;
    
    for (int i = 0; i < total_data_blks - 1; i++) {         // file extends are 0 based
        Blocknum to_erase = get_file_extend(inode, i);
        if (free_block(to_erase) < 0)
            RET_ERROR(BLOCK_WRITE_FAILED);
    }

    // Erase inode extends
    int num_of_inode_extends = 0;
    if (total_data_blks > INODE_MAX_FE) {
        int extra_data_blocks = total_data_blks - INODE_MAX_FE;         // Number of data block ptrs that didn't fit in inode
        num_of_inode_extends += extra_data_blocks / (FE_MAX_DATA / sizeof(BLOCKSIZE));

        if (extra_data_blocks % (FE_MAX_DATA / sizeof(BLOCKSIZE)) > 0)  // Count the extra block needed to store left over
            num_of_inode_extends += 1;
    }

    if (num_of_inode_extends != 0) {
        for (int i = num_of_inode_extends - 1; i > 0; i--) {
            for (int j = 0; j < i; j++) {
                if (readBlock(mountedDisk, inode[INODE_EXTEND], inode) < 0) 
                    RET_ERROR(FAILED_TO_READ);
            }

            free_block(inode[INODE_EXTEND]);
            if (readBlock(mountedDisk, entry->inode, inode) < 0) 
                RET_ERROR(FAILED_TO_READ);
        }
        
        free_block(inode[INODE_EXTEND]);
    } 

    // Erase inode
    if (free_block(entry->inode) < 0)
            RET_ERROR(BLOCK_WRITE_FAILED);

    // Erase file entry
    removed_file_entry_from_directory(entry->fileName);
    removeEntry(openedFilesList, FD);
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
    if (readBlock(mountedDisk, entry->inode, tempBlock) < 0)   // Read inode data
        RET_ERROR(FAILED_TO_READ);

    // Find block that contains requested data
    Blocknum file_ext = entry->cursor / FE_MAX_DATA;    // logical block that fp is at. logblk = cur / data per fe blk
    int loc      = entry->cursor % FE_MAX_DATA;         // Location of fp in file extend

    Blocknum physical_block = get_file_extend(tempBlock, file_ext); 
    if (physical_block <= 0)
        RET_ERROR(FAILED_TO_READ);

    if (readBlock(mountedDisk, physical_block, tempBlock) < 0)
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