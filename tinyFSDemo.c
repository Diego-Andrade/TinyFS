#include <stdlib.h>
#include <stdio.h>

#include "libTinyFS.h"
#include "tinyFS.h"
#include "tinyFS_errno.h"

#define A_SIZE 912  //4 blocks 3 full + 150
#define B_SIZE 145  //1 Block   145 

// Writing file with for loop index data
int writeFile(int fd, int size) {
    char buffer[size];
    for (int i = 0; i < size; i++) {
        buffer[i] = i;
    }

    if (tfs_writeFile(fd, buffer, size) < 0)
        return -10;

    return 0;
}

int verifyFile(int fd, int start, int end) {
    for (int i = start; i < end; i++) {
        unsigned char buffer;
        
        if (tfs_readByte(fd, &buffer) < 0)
            return -10;

        if (buffer != i)
            return -10;
    }
    return 0;
}

int main() {
    /* Our demo code? */
    int fdA;
    int fdB;

    char bufferA[A_SIZE];
    for (int i = 0; i< A_SIZE; i++) {
        bufferA[i] = 'B';
    }

    if (tfs_mkfs(DEFAULT_DISK_NAME, BLOCKSIZE*9) < 0)
        return -10;
    if (tfs_mount(DEFAULT_DISK_NAME) < 0)
        return -10;
    
    // Writing file A
    if ((fdA = tfs_openFile("AFile")) < 0)
        return -10;
    if (tfs_writeFile(fdA, bufferA, A_SIZE) < 0)
        return -10;
    for (int i = 0; i < A_SIZE; i++) {
        unsigned char buffer;
        
        if (tfs_readByte(fdA, &buffer) < 0)
            return -10;

        if (buffer != 'A')
            return -10;
    }    

    // B file
    if ((fdB = tfs_openFile("BFile")) < 0)
        return -10;
    if (writeFile(fdB, B_SIZE) < 0)
        return -10;
    
    if (tfs_seek(fdB, -100) < 0)
        return -10;  
    if (verifyFile(fdB, B_SIZE - 100, B_SIZE) < 0)
        return -10;


    // C file
    int fdC;
    if ((fdC = tfs_openFile("CFile")) < 0)
        return -10;
    if (writeFile(fdC, 300) < 0)
        return -10;
    if (verifyFile(fdC, 0, 300) < 0)
        return -10;

    // Close
    if (tfs_readdir() < 0)
        return -10;
    if (tfs_closeFile(fdB) < 0)
        return -10;
    if (tfs_readdir() < 0)
        return -10;

    // Raname
    if ((fdB = tfs_openFile("BFile")) < 0)
        return -10;        
    if (tfs_rename(fdB, "DFile") < 0)
        return -10;
    if (tfs_readdir() < 0)
        return -10;

    // Delete
    if ((fdB = tfs_deleteFile(fdB)) < 0)
        return -10;
    if (tfs_readdir() < 0)
        return -10;

    
    if ((fdB = tfs_openFile("BFile")) < 0)
        return -10;
    if (tfs_readdir() < 0)
        return -10;
    if (writeFile(fdB, B_SIZE) >= 0)
        return -10;
    if (verifyFile(fdB, 0, 300) < 0)
        return -10;

    if (tfs_unmount() < 0)
        return -10;
    return 0;

   

}



