#include <stdlib.h>
#include <stdio.h>

#include "libDisk.h"

int main(int argc, char const *argv[])
{
    /* Our demo code? */
    
    int d = openDisk("Disk1.dsk", 256 * 5);
    int e = openDisk("Disk2.dsk", 256 * 1);
    int f = openDisk("Disk1.dsk", 0);

    printf("Disk1.dsk num: %d\n", d);
    printf("Disk2.dsk num: %d\n", e);
    printf("Disk1.dsk num: %d\n", f);

    // for (int i = 0; i < 256; i++)
        

    closeDisk(d);
    closeDisk(e);

    return 0;
}
