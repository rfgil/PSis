#include <stdlib.h>

#include "headed_list.h"

struct list_node{
  void * item;
  struct list_node * next;
} ;

struct list_head{
    int n_elements;
    int (*compareItemWithId) (void *, void *);
    void (*freeItem) (void *);
    ListNode * first_node;
    ListNode * current_node;
};

static int defaultCompareItemWithId(void * a, void * b){return GREATER;}
static void defaultFreeItem(void * item){}

List * newList(int (*compareItemWithId)(void *, void *), void (*freeItem) (void *)){
  List * new_list;

  new_list = (List *) malloc(sizeof(List));
  new_list->n_elements = 0;
  new_list->first_node = NULL;
  new_list->current_node = NULL;

  new_list->compareItemWithId = compareItemWithId == NULL ? defaultCompareItemWithId : compareItemWithId;
  new_list->freeItem = freeItem == NULL ? defaultFreeItem : freeItem;

  return new_list;
}

static void freeListNodes(ListNode * list_node, void (*freeItem)(void *)){
  if (list_node != NULL){
    freeListNodes(list_node->next, freeItem);
    freeItem(list_node->item);
    free(list_node);
  }
}
void freeList(List * list){
  if (list != NULL){
    freeListNodes(list->first_node, list->freeItem);
    free(list);
  }
}


void insertListItem(List * list, void * item, void * item_id){
  ListNode * new_list_node;

  new_list_node = (ListNode *) malloc(sizeof(ListNode));
  new_list_node->item = item;

  list->n_elements ++;

  // Verifica se deve introduzir no inicio da lista
  if(list->first_node == NULL || list->compareItemWithId(list->first_node, item_id) != SMALLER){
    // mutex aqui
    new_list_node->next = list->first_node;
    list->first_node = new_list_node;
    return;
  }

  ListNode * prev, * aux = list->first_node;

  do {
    prev = aux;
    aux = aux->next;
  } while(aux != NULL && list->compareItemWithId(aux->item, item_id) == SMALLER);

  new_list_node->next = aux;
  prev->next = new_list_node;
}

// Estas funções devolvem NULL se for usada a função defaultCompareItems
void * removeListItem(List * list, void * id){
  ListNode * aux = list->first_node;
  ListNode * prev;
  void * item;
  int comparsion;

  // O elemento encontra-se no inicio da lista
  if (aux != NULL && list->compareItemWithId(aux->item, id) == EQUAL){
    item = aux->item;

    // mutex aqui
    list->n_elements --;
    list->first_node = aux->next;
    free(aux);

    // O utilizador é responsavel pelo free do item
    return item;
  }

  do {
    prev = aux;
    aux = aux->next;
  } while(aux != NULL && (comparsion = list->compareItemWithId(aux->item, id)) == SMALLER);

  if (comparsion == EQUAL){
    item = aux->item;

    // mutex aqui
    list->n_elements --;
    prev->next = aux->next;
    free(aux);

    // O utilizador é responsavel pelo free do item
    return item;
  }

  return NULL;
}
void * findListItem(List * list, void * id){
  ListNode * aux = list->first_node;
  int comparsion = SMALLER;

  // Avança enquanto aux for válido e o id do elemento actual for meno que id
  // (a lista está ordenada por ordem crescente de ids)
  while(aux != NULL && (comparsion = list->compareItemWithId(aux->item, id)) == SMALLER){
    aux = aux->next;
  }

  return comparsion == EQUAL ? aux->item : NULL;
}

ListNode * getFirstListNode(List * list){
  return list->first_node;
}
ListNode * getNextListNode(ListNode * list_node){
  return list_node->next;
}
void * getListNodeItem(ListNode * list_node){
  return list_node->item;
}

int getListSize(List * list){
  return list->n_elements;
}
