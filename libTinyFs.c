#include "libTinyFS.h"

#include <stdio.h>
#include <string.h>
#include "libDisk.h"

int tfs_mkfs(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE * 2) return -1;  // Not enough bytes to make a disk

    int d = openDisk(filename, nBytes);

    if (d < 0) return -1;                   // Failed to open disk

    int num_blocks = getDiskSize(d);

    unsigned char buffer[BLOCKSIZE];

    // Superblock
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = SUPERBLOCK;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    buffer[2] = num_blocks - 1;         // Number of empty blocks
    buffer[3] = 1;                      // Empty block ptr
    writeBlock(d, 0, buffer);
    
    // Empty blocks
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = FREE;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    for (int i = 1; i < num_blocks; i++)
        writeBlock(d, i, buffer);

    // Possible single inode?
    
    if (closeDisk(d) < 0)
        return -1;

    return 0;
}

int tfs_mount(char *diskname);
int tfs_unmount(void);

fileDescriptor tfs_openFile(char *name);

int tfs_closeFile(fileDescriptor FD);

int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset);