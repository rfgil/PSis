#include <stdlib.h>

#include "headed_list.h"

typedef struct list_node{
  void * item;
  struct list_node * next;
} ListNode;

struct list_head{
    int n_elements;
    ListNode * first_node;
    ListNode * current_node;
};

List * newList(){
  List * new_list;

  new_list = (List *) malloc(sizeof(List));
  new_list->n_elements = 0;
  new_list->first_node = NULL;
  new_list->current_node = NULL;

  return new_list;
}

void insertList(List * list, void * item){
  ListNode * new_list_node;

  new_list_node = (ListNode *) malloc(sizeof(ListNode));
  new_list_node->item = item;
  new_list_node->next = list->first_node;

  list->n_elements ++;
  list->first_node = new_list_node;
}

static void freeListNodes(ListNode * list_node, void (*freeItem)(void *)){
  if (list_node != NULL){
    freeListNodes(list_node->next, freeItem);
    freeItem(list_node->item);
    free(list_node);
  }
}

void freeList(List * list, void (*freeItem)(void *)){
  if (list != NULL){
    freeListNodes(list->first_node, freeItem);
    free(list);
  }
}

void startListIteration(List * list){
  list->current_node = list->first_node;
}

void * getListNextItem(List * list){
  void * item = NULL;

  if (list->current_node != NULL){
    item = list->current_node->item;
    list->current_node = list->current_node->next;
  }

  return item;
}

int getListSize(List * list){
  return list->n_elements;
}
