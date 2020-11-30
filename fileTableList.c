#include "fileTableList.h"

#define EMPTY_LIST -6

LList *createTableList()
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

Node *makeNewNode(char* fileName, int size, int fd)
{
   Node *newNode;
   if((newNode = malloc(sizeof(Node))) == NULL)
      {
         perror("makeNewNode");
         exit(EXIT_FAILURE);
      }
   newNode->fileName = (char*)malloc(strlen(fileName));
   strcpy(newNode->fileName, fileName);
   newNode->size = size;
   newNode->fd = fd;
   return newNode;
}

int registerEntry(LList *list, char* fileName, int size, int fd)
{
   Node *newNode;

   if (list == NULL)
      return EMPTY_LIST;
   newNode = makeNewNode(fileName, size, fd);
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
   return 0;
}

int removeEntry(LList *list, int fd)
{
   Node *prevNode;
   Node *currNode;

   if (list == NULL || list->head == NULL)
      return EMPTY_LIST;
   currNode = list->head;
   prevNode = list->tail;
   while (currNode->fd != fd && currNode != list->tail)
   {
      prevNode = currNode;
      currNode = currNode->nextNode;
   }
   if (currNode->fd == fd)
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
      free(currNode->fileName);
      free(currNode);
      list->numEntries -= 1;
      return 0;
   }
   return -1;
}

void printTable(LList *list)
{
   Node *currNode;
   int fullRound = 0;
   if (list == NULL || list->head == NULL)
   {
      printf("NULL\n");
      return;
   }
   currNode = list->head;
   while (!fullRound)
   {
      printf("(%s,%d)->", currNode->fileName, currNode->fd);
      currNode = currNode->nextNode;
      if (currNode == list->head)
         fullRound = 1;
   }
   printf("\n");
}

void purgeTable(LList *list)
{
   Node *prevNode;
   Node *currNode;

   if (list == NULL)
      return;
   currNode = list->head;
   prevNode = list->head;
   while(currNode != list->tail)
   {
      currNode = currNode->nextNode;
      free(prevNode->fileName);
      free(prevNode);
      prevNode = currNode;
   }
   if (currNode != NULL)
   {
      free(currNode->fileName);
      free(currNode);
   }
   free(list);
}

Node *findEntry_fd(LList *list, int fd)
{
   Node *currNode;

   if (list == NULL)
      return NULL;
   currNode = list->head;
   while(currNode != list->tail && currNode->fd != fd)
      currNode = currNode->nextNode;
   if (currNode->fd == fd)
      return currNode;
   return NULL;
}

Node *findEntry_name(LList *list, char* filename)
{
   Node *currNode;

   if (list == NULL || list->head == NULL)
      return NULL;
   currNode = list->head;
   while(currNode != list->tail && strcmp(currNode->fileName, filename) != 0)
      currNode = currNode->nextNode;
   if (strcmp(currNode->fileName, filename) == 0)
      return currNode;
   return NULL;
}