#ifndef HEADED_LIST_H_INCLUDED
#define HEADED_LIST_H_INCLUDED

typedef struct list_head List;

List * newList();
void insertList(List * list, void * item);
void freeList(List * list, void (*freeItem)(void *));

void startListIteration(List * list);
void * getListNextItem(List * list);

int getListSize(List * list);

#endif
