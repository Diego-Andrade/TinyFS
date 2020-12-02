/* You must specify a set of unified error codes returned by your TinyFS interfaces. 
All error codes must be negative integers (-1 or lower), but it is up to you to 
assign specific meaning to each. Error codes must be informational only, and not 
used as status in subsequent conditionals. Create a file called tinyFS_errno.h and 
implement the codes as a set of statically defined macros. Take a look at man 3 errno 
on the UNIX* machines for examples of the types of errors you might catch and report. */
#include <stdio.h>

#define RET_ERROR(x) {\
   int X = (x); char error[30];\
   if (X == DISK_NOT_FOUND) strcpy(error, "DISK NOT FOUND");\
   else if (X == INSUFFICIENT_SPACE) strcpy(error, "INSUFFICIENT SPACE");\
   else if (X == FAILURE_TO_OPEN) strcpy(error, "FAILURE TO OPEN");\
   else if (X == FILE_NOT_FOUND) strcpy(error, "FILE NOT FOUND");\
   else if (X == INVALID_NAME) strcpy(error, "INVALID NAME");\
   else if (X == EMPTY_LIST) strcpy(error, "EMPTY LIST");\
   else if (X == BYTES_SMALLER_THAN_BLOCKSIZE) strcpy(error, "BYTES SMALLER THAN BLOCKSIZE");\
   else if (X == FORMAT_ISSUE) strcpy(error, "FORMAT ISSUE");\
   else if (X == OUT_OF_BOUNDS) strcpy(error, "OUT OF BOUNDS");\
   else if (X == FILE_NULL) strcpy(error, "FILE NULL");\
   else if (X == INVALID_FD) strcpy(error, "INVALID FILE DESCRIPTOR");\
   else if (X == INVALID_FILE_OFFSET) strcpy(error, "INVALID FILE OFFSET");\
   else if (X == FAILED_TO_READ) strcpy(error, "UNABLE TO READ FILE");\
   else if (X == TFS_EOF) strcpy(error, "ATTEMPED TO READ PAST FILE");\
   else strcpy(error, "UNKNOWN ERROR - ");\
   fprintf(stderr, "tinyFS ERROR: %s\n", error); fflush(stderr);\
   return X; }

#define DISK_NOT_FOUND -3
#define INSUFFICIENT_SPACE -4
#define FAILURE_TO_OPEN -5
#define FILE_NOT_FOUND -6
#define INVALID_NAME -8
#define EMPTY_LIST -9

#define BYTES_SMALLER_THAN_BLOCKSIZE -10
#define FORMAT_ISSUE -11
#define OUT_OF_BOUNDS -12
#define FILE_NULL -13
#define INVALID_FD -14
#define INVALID_FILE_OFFSET -15

#define FAILED_TO_READ -16
#define TFS_EOF -17
#define BLOCK_WRITE_FAILED -18
