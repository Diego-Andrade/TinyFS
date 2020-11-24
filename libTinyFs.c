#include "libTinyFS.h"

#include <stdio.h>
#include <string.h>
#include "libDisk.h"

int tfs_mkfs(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE * 2) return -1;  // Not enough bytes to make a disk

    int d = openDisk(filename, nBytes);

    if (d < 0) return -1;       // Failed to open disk

    unsigned char buffer[BLOCKSIZE];

    // Superblock
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = SUPERBLOCK;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    buffer[3] = 1;      // Empty block ptr


    writeBlock(d, 0, buffer);
    
    // TODO: Necessary?
    // Empty blocks
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = FREE;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    for (int i = 1; i < nBytes / BLOCKSIZE; i++)
        writeBlock(d, i, buffer);

    // Possible single inode?
    
    closeDisk(d);

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