#ifndef LINKEDLIST_H
#define LINKEDLIST_H
#include <stdio.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct node 
{
   pid_t pid;
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
Node *makeNewNode(pid_t pid);
void insertNewNodeTail(LList *list, pid_t pid);
void removeNode(LList *list, pid_t pid);
void printNodes(LList *list);
#endif
