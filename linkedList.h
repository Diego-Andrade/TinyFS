#ifndef LINKEDLIST_H
#define LINKEDLIST_H
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct node 
{
   char* fileName;
   int numBlocks;
   int diskNum;
   struct node *nextNode;
} Node;

typedef struct
{
   Node *head;
   Node *tail;
   int numEntries;
} LList;

void purgeList(LList *list);
LList *createLinkedList();
void registerDisk(LList *list, int diskNum, char* fileName, int blockNum);
int getDiskNum(LList *list, char* filename);
void removeNode(LList *list, int diskNum);
void printNodes(LList *list);
Node *getNode(LList *list, int diskNum);
#endif