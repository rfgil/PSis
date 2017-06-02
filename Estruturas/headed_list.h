#ifndef HEADED_LIST_H_INCLUDED
#define HEADED_LIST_H_INCLUDED

typedef struct list_head List;
typedef struct list_node ListNode;

#define SMALLER -1
#define EQUAL 0
#define GREATER 1

List * newList(int (*compareItemWithId)(void *, void *), void (*freeItem) (void *));
void freeList(List * list);

void insertListItem(List * list, void * item, void * item_id);
void * removeListItem(List * list, void * id);
void * findListItem(List * list, void * id);

ListNode * getFirstListNode(List * list);
ListNode * getNextListNode(ListNode * list_node);
void * getListNodeItem(ListNode * list_node);

int getListSize(List * list);

#endif
