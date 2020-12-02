#include <stdlib.h>
#include <stdio.h>

#include "libTinyFS.h"
#include "tinyFS.h"
#include "tinyFS_errno.h"

#define A_SIZE 912  //4 blocks 3 full + 150
#define B_SIZE 245  //1 Block   245 
int main() {
    /* Our demo code? */
    int fdA;
    int fdB;
    char bufferA[A_SIZE];
    char bufferB[B_SIZE];
    int i;
    for (i = 0; i< A_SIZE; i++)
    {
        bufferA[i] = 'A';
    }
    for (i = 0; i< B_SIZE; i++)
    {
        bufferB[i] = 'B';
    }

    if (tfs_mkfs(DEFAULT_DISK_NAME, BLOCKSIZE*9) < 0)
        return -10;
    if (tfs_mount(DEFAULT_DISK_NAME) < 0)
        return -10;
    if ((fdA = tfs_openFile("AFile")) < 0)
        return -10;
    if ((fdB = tfs_openFile("BFile")) < 0)
        return -10;
    if (tfs_writeFile(fdA, bufferA, A_SIZE) < 0)
        return -10;
    if (tfs_writeFile(fdB, bufferB, B_SIZE) < 0)
        return -10;
    if (tfs_unmount() < 0)
        return -10;
    return 0;

    // if (tfs_closeFile(fd) < 0)
    //     return -10;

}



