/* You must specify a set of unified error codes returned by your TinyFS interfaces. 
All error codes must be negative integers (-1 or lower), but it is up to you to 
assign specific meaning to each. Error codes must be informational only, and not 
used as status in subsequent conditionals. Create a file called tinyFS_errno.h and 
implement the codes as a set of statically defined macros. Take a look at man 3 errno 
on the UNIX* machines for examples of the types of errors you might catch and report. */
#define DISK_NOT_FOUND_TO_MOUNT -2
#define DISK_NOT_FOUND -3
#define INSUFFICIENT_SPACE -4
#define FAILURE_TO_OPEN -5
#define READ_ERROR -6
#define WRITE_ERROR -7
#define INVALID_NAME -8
#define NO_MORE_SPACE -9