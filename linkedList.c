#include "linkedList.h"

#define EMPTY_LIST -4

LList *createLinkedList()
{
   LList *list;

   if ((list = malloc(sizeof(LList))) == NULL)
   {
      perror("LinkedList");
      exit(EXIT_FAILURE);
   }
   list->head = NULL;
   list->tail = NULL;
   list->numEntries = 0;
   return list;
}

Node *makeNewNode(int diskNum, char* fileName, int numBlocks)
{
   Node *newNode;
   if((newNode = malloc(sizeof(Node))) == NULL)
      {
         perror("makeNewNode");
         exit(EXIT_FAILURE);
      }
   newNode->diskNum = diskNum;
   newNode->fileName = fileName;
   newNode->numBlocks = numBlocks;  
   newNode->nextNode = newNode;
   return newNode;
}

void registerDisk(LList *list, int diskNum, char* fileName, int numBlocks)
{
   Node *newNode = makeNewNode(diskNum, fileName, numBlocks);
   if (list->head == NULL)
   {
      list->head = newNode;
      list->tail = newNode;
   }
   else
   {
      list->tail->nextNode = newNode;
      newNode->nextNode = list->head;
      list->tail = newNode;
   }
   list->numEntries += 1;
}

int removeNode(LList *list, int diskNum)
{
   Node *prevNode = list->tail;
   Node *currNode = list->head;

   if (list->head == NULL)
      return EMPTY_LIST;
   while (currNode->diskNum != diskNum && currNode != list->tail)
   {
      prevNode = currNode;
      currNode = currNode->nextNode;
   }
   if (currNode->diskNum == diskNum)
   {
      if (list->numEntries == 1) /*one element*/
      {
         list->tail = NULL;
         list->head = NULL;
      }
      else
         prevNode->nextNode = currNode->nextNode;
      if (currNode == list->head)
         list->head = currNode->nextNode;
      else if (currNode == list->tail)
         list->tail = prevNode;
      free(currNode);
      list->numEntries -= 1;
      return 0;
   }
   return -1;
}

void printNodes(LList *list)
{
   Node *currNode = list->head;
   int fullRound = 0;
   if (currNode == NULL)
   {
      printf("NULL\n");
      return;
   }
   while (!fullRound)
   {
      printf("%d->", currNode->diskNum);
      currNode = currNode->nextNode;
      if (currNode == list->head)
         fullRound = 1;
   }
   printf("\n");
}

void purgeList(LList *list)
{
   Node *prevNode = list->head;
   Node *currNode = list->head;

   if (currNode == NULL)
      return;
   while(currNode != list->tail)
   {
      currNode = currNode->nextNode;
      free(prevNode);
      prevNode = currNode;
   }
   if (currNode != NULL)
      free(currNode);
   free(list);
}

//Returns Null if not found
Node *getNode(LList *list, int diskNum)
{
   Node *currNode = list->head;

   if (currNode == NULL)
      return NULL;

   while(currNode != list->tail)
   {
      if (currNode->diskNum == diskNum)
         return currNode;
      currNode = currNode->nextNode;
   }
   return NULL;
}

int getDiskNum(LList *list, char* filename)
{
   Node *currNode = list->head;

   if (currNode == NULL)
      return EMPTY_LIST;
   while(currNode != list->tail)
   {
      if (strcmp(currNode->fileName, filename) == 0)
         return currNode->diskNum;
      currNode = currNode->nextNode;
   }
   return -1;
}