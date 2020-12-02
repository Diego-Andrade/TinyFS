#include "fileTableList.h"

#define EMPTY_LIST -6

FileTable *createFileTable()
{
   FileTable *list;

   if ((list = malloc(sizeof(FileTable))) == NULL)
   {
      perror("LinkedList");
      exit(EXIT_FAILURE);
   }
   list->head = NULL;
   list->tail = NULL;
   list->numEntries = 0;
   return list;
}

FileEntry *makeNewEntry(char* fileName, Bytes2_t inode, int size, int fd)
{
   FileEntry *newNode;
   if((newNode = malloc(sizeof(FileEntry))) == NULL)
      {
         perror("makeNewEntry");
         exit(EXIT_FAILURE);
      }
   newNode->fileName = (char*)malloc(strlen(fileName));
   strcpy(newNode->fileName, fileName);
   newNode->inode = inode;
   newNode->cursor = 0;
   newNode->size = size;
   newNode->fd = fd;
   return newNode;
}

int registerEntry(FileTable *list, char* fileName, Bytes2_t inode, int size, int fd)
{
   FileEntry *newNode;

   if (list == NULL)
      return EMPTY_LIST;
   newNode = makeNewEntry(fileName, inode, size, fd);
   if (list->head == NULL)
   {
      list->head = newNode;
      list->tail = newNode;
   }
   else
   {
      list->tail->next = newNode;
      newNode->next = list->head;
      list->tail = newNode;
   }
   list->numEntries += 1;
   return 0;
}

int removeEntry(FileTable *list, int fd)
{
   FileEntry *prevNode;
   FileEntry *currNode;

   if (list == NULL || list->head == NULL)
      return EMPTY_LIST;
   currNode = list->head;
   prevNode = list->tail;
   while (currNode->fd != fd && currNode != list->tail)
   {
      prevNode = currNode;
      currNode = currNode->next;
   }
   if (currNode->fd == fd)
   {
      if (list->numEntries == 1) /*one element*/
      {
         list->tail = NULL;
         list->head = NULL;
      }
      else
         prevNode->next = currNode->next;
      if (currNode == list->head)
         list->head = currNode->next;
      else if (currNode == list->tail)
         list->tail = prevNode;
      free(currNode->fileName);
      free(currNode);
      list->numEntries -= 1;
      return 0;
   }
   return -1;
}

void printTable(FileTable *list)
{
   FileEntry *currNode;
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
      currNode = currNode->next;
      if (currNode == list->head)
         fullRound = 1;
   }
   printf("\n");
}

void purgeTable(FileTable *list)
{
   FileEntry *prevNode;
   FileEntry *currNode;

   if (list == NULL)
      return;
   currNode = list->head;
   prevNode = list->head;
   while(currNode != list->tail)
   {
      currNode = currNode->next;
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

FileEntry *findEntry_fd(FileTable *list, int fd)
{
   FileEntry *currNode;

   if (list == NULL)
      return NULL;
   currNode = list->head;
   while(currNode != list->tail && currNode->fd != fd)
      currNode = currNode->next;
   if (currNode->fd == fd)
      return currNode;
   return NULL;
}

FileEntry *findEntry_name(FileTable *list, char* filename)
{
   FileEntry *currNode;

   if (list == NULL || list->head == NULL)
      return NULL;
   currNode = list->head;
   while(currNode != list->tail && strcmp(currNode->fileName, filename) != 0)
      currNode = currNode->next;
   if (strcmp(currNode->fileName, filename) == 0)
      return currNode;
   return NULL;
}