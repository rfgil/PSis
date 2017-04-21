#include <stdlib.h>

#include "list.h"

List * newList(){
  return NULL;
}

List * insertList(List * list, void * item){
  List * new;

  new = malloc(sizeof(List));
  new->item = item;
  new->next = list;

  return new;
}

void freeList(List * element, void (*freeItem)(void *)){
  if (element != NULL){
    freeList(element->next, freeItem);
    freeItem(element->item);
    free(element);
  }
}

List * removeFirstList(List * list, void ** item){
  List * head;

  if (list != NULL){
    (*item) =  list->item;
    head = list->next;
    free(list);
    return head;;
  }
  return NULL;
}


int findListItem(List * list, void * item){
  while (list != NULL ){
    if(list->item == item){
      return TRUE;
    }
    list = list->next;
  }
  return FALSE;
}

