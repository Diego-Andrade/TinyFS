#include <stdlib.h>
#include <stdio.h>

#include "libDisk.h"

int main(int argc, char const *argv[])
{
    /* Our demo code? */
    
    int d = openDisk("Disk1.dsk", 256 * 5);

   
    // for (int i = 0; i < 256; i++)
        

    closeDisk(d);

    return 0;
}
