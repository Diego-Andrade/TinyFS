#ifndef TINYFS_H
#define TINYFS_H

/*** File System info ***/
#define BLOCKSIZE 256           // Amount of Bytes per block
#define MAGICNUMBER 0x44        // Used to identify formating
#define MAX_FILENAME_SIZE 8     // Max size of filename excluding null terminator

typedef int fileDescriptor;
typedef int Bytes4_t;
typedef signed short Bytes2_t;      // 2 Byte long int
typedef unsigned short Blocknum;    // 2 Byte Block pointer

/*** User of lib helpers ***/
#define DEFAULT_DISK_SIZE 10240
#define DEFAULT_DISK_NAME "tinyFSDisk"


#endif