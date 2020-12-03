/* You must specify a set of unified error codes returned by your TinyFS interfaces. 
All error codes must be negative integers (-1 or lower), but it is up to you to 
assign specific meaning to each. Error codes must be informational only, and not 
used as status in subsequent conditionals. Create a file called tinyFS_errno.h and 
implement the codes as a set of statically defined macros. Take a look at man 3 errno 
on the UNIX* machines for examples of the types of errors you might catch and report. */
#include <stdio.h>

///* VERBOS
#define RET_ERROR(x) {\
   int X = (x); char error[30];\
   if (X == DISK_NOT_FOUND) strcpy(error, "DISK NOT FOUND");\
   else if(X == DISK_OPEN_FAILED) strcpy(error, "DISK FAILED TO OPEN");\
   else if(X == DISK_INVALID_SIZE) strcpy(error, "INVALID DISK SIZE");\
   else if(X == FILE_NOT_FOUND) strcpy(error, "FILE NOT FOUND");\
   else if(X == FILE_OPEN_FAILED) strcpy(error, "FILE FAILED TO OPEN");\
   else if(X == FILE_INVALID_NAME) strcpy(error, "INVALID FILE NAME");\
   else if(X == FILE_INVALID_FD) strcpy(error, "INVALID FILEDESCRIPTER");\
   else if(X == FILE_INVALID_OFFSET) strcpy(error, "INVAID FILE OFFSET");\
   else if(X == BLOCK_INVALID ) strcpy(error, "INVALID BLOCK");\
   else if(X == BLOCK_FORMAT_ISSUE ) strcpy(error, "UNEXPECTED BLOCK FORMATE");\
   else if(X == BLOCK_READ_FAILED ) strcpy(error, "FAILED TO READ BLOCK");\
   else if(X == BLOCK_WRITE_FAILED ) strcpy(error, "FAILED TO WRITE BLOCK");\
   else if(X == OUT_OF_BOUNDS ) strcpy(error, "OUT OF BOUNDS");\
   else if(X == TFS_EOF ) strcpy(error, "END OF FILE");\
   else strcpy(error, "UNKOWN ERROR");\
   fprintf(stderr, "tinyFS ERROR: %s\n", error); fflush(stderr);\
   return X; }

//*/

#define DISK_NOT_FOUND -2
#define DISK_OPEN_FAILED -3
#define DISK_INVALID_SIZE -4

#define FILE_NOT_FOUND -5
#define FILE_OPEN_FAILED -6
#define FILE_INVALID_NAME -7
#define FILE_INVALID_FD -8
#define FILE_INVALID_OFFSET -9

#define BLOCK_INVALID -10
#define BLOCK_FORMAT_ISSUE -11
#define BLOCK_READ_FAILED -12
#define BLOCK_WRITE_FAILED -13

#define OUT_OF_BOUNDS -14
#define TFS_EOF -15
