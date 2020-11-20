#include <stdlib.h>
#include <stdio.h>

//#include "libTinyFS.h"

int main(int argc, char const *argv[])
{
    /* Our demo code? */
    
    FILE* file = fopen("test.dsk", "wb+");

   
    for (int i = 0; i < 256; i++)
        fprintf(file, "0");
    fclose(file);

    return 0;
}
