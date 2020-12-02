#ifndef FILETABLELIST_H
#define FILETABLELIST_H
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tinyFS.h"

typedef struct FileEntry 
{
   char* fileName;   // Name of file
   Bytes2_t inode;   // Block number of inode
   int cursor;       // Current file ptr location in file
   int size;         
   int fd;           // Index in FileTable
   struct FileEntry *next;  
} FileEntry;

typedef struct FileTable
{
   FileEntry *head;
   FileEntry *tail;
   int numEntries;
} FileTable;

FileTable *createFileTable();
int registerEntry(FileTable *table, char* fileName, Blocknum inode, int size, int fd);
FileEntry *findEntry_fd(FileTable *table, int fd);
FileEntry *findEntry_name(FileTable *table, char* filename);
int removeEntry(FileTable *table, int fd);
void printTable(FileTable *table);
void purgeTable(FileTable *table);
#endif