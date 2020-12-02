#include "tinyFS.h"

// Reserved disk block numbers
#define SUPERBLOCK_BNUM 0
#define RINODE_BNUM 1

// Block types
#define SUPERBLOCK 1
#define INODE 2
#define FILEEXTEND 3
#define FREE 4

// General block format
#define BLOCKTYPELOC 0  // Required
#define MAGICNUMLOC 1   // Req

// Superblock
#define SUPER_TYPE 1
#define SUPER_FREE_LIST 2  
#define SUPER_FREE_COUNT (SUPER_FREE_LIST + sizeof(Blocknum))  // Next item after free list link, which is blocknum size

//Inode Struct
#define INODE_TYPE 2
#define INODE_NAME_START 2
#define INODE_SIZE_START 11
#define INODE_BLOCKS_START 13
#define INODE_DATA_START 15

#define INODE_FE_LIST 15
#define INODE_EXTEND (BLOCKSIZE - sizeof(Blocknum))
#define INODE_MAX_FE ((INODE_EXTEND - INODE_FE_LIST) / sizeof(Blocknum))

// Root Inode
#define FILE_ENTRY_SIZE (MAX_FILENAME_SIZE + 1 + 2)      //+1 for null char, +2 for the two point block numbers

// File Extend
#define FE_TYPE 3
#define FE_DATA 2                               // Start of data 
#define FE_MAX_DATA (BLOCKSIZE - FE_DATA)       // Max storable amount of data

// Free 
#define FREE_TYPE 4
#define FREE_DATA_START 2


/* Makes a blank TinyFS file system of size nBytes on the unix file specified by 
‘filename’. This function should use the emulated disk library to open the specified 
unix file, and upon success, format the file to be a mountable disk. This includes 
initializing all data to 0x00, setting magic numbers, initializing and writing the 
superblock and inodes, etc. Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes);

/* tfs_mount(char *diskname) “mounts” a TinyFS file system located within ‘diskname’. 
tfs_unmount(void) “unmounts” the currently mounted file system. As part of the mount 
operation, tfs_mount should verify the file system is the correct type. In tinyFS, only 
one file system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently
 mounted file system. Must return a specified success/error code. */
int tfs_mount(char *diskname);
int tfs_unmount(void);

/* Creates or Opens a file for reading and writing on the currently mounted file system. 
Creates a dynamic resource table entry for the file, and returns a file descriptor (integer) 
that can be used to reference this entry while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name);

/* Closes the file, de-allocates all system resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD);

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, 
to the file system. Previous content (if any) will be completely lost. Sets the file 
pointer to 0 (the start of file) when done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD);

/* reads one byte from the file and copies it to buffer, using the current file pointer 
location and incrementing it by one upon success. If the file pointer is already past the end 
of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset);