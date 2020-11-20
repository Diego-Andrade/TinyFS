#include "linkedList.h"

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

Node *makeNewNode(pid_t pid)
{
   Node *newNode;
   if((newNode = malloc(sizeof(Node))) == NULL)
      {
         perror("makeNewNode");
         exit(EXIT_FAILURE);
      }
   newNode->pid = pid;   
   newNode->nextNode = newNode;
   return newNode;
}

void insertNewNodeTail(LList *list, pid_t pid)
{
   Node *newNode = makeNewNode(pid);
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

void removeNode(LList *list, pid_t pid)
{
   Node *prevNode = list->tail;
   Node *currNode = list->head;

   if (list->head == NULL)
      return;
   while (currNode->pid != pid && currNode != list->tail)
   {
      prevNode = currNode;
      currNode = currNode->nextNode;
   }
   if (currNode->pid == pid)
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
   }
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
      printf("%d->", currNode->pid);
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
