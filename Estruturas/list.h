#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

#define TRUE 1
#define FALSE 0

typedef struct element{
  void * item;
  struct element * next;
} List;

List * newList();
List * insertList(List * list, void * item);
void freeList(List * list, void (*freeItem)(void *));

List * removeFirstList(List * list, void ** item);

int findListItem(List * list, void * item);

#endif

