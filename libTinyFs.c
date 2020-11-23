#include "libTinyFS.h"

#include <stdio.h>
#include <string.h>
#include "libDisk.h"

int tfs_mkfs(char *filename, int nBytes) {
    int d = openDisk(filename, nBytes);
    
    //superblock data
    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);
    buffer[BLOCKTYPELOC] = SUPERBLOCK;
    buffer[MAGICNUMLOC] = MAGICNUMBER;
    writeBlock(d, 0, buffer);
    
    // Write empty blocks?

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