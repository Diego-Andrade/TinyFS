#ifndef FILETABLELIST_H
#define FILETABLELIST_H
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct node 
{
   char* fileName;
   int size;
   int fd;
   struct node *nextNode;
} Node;

typedef struct
{
   Node *head;
   Node *tail;
   int numEntries;
} LList;

LList *createTableList();
int registerEntry(LList *list, char* fileName, int size, int fd);
Node *findEntry_fd(LList *list, int fd);
Node *findEntry_name(LList *list, char* filename);
int removeEntry(LList *list, int fd);
void printTable(LList *list);
void purgeTable(LList *list);
#endif