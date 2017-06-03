#include <stdlib.h>
#include <pthread.h>
#include "headed_list.h"

struct list_node{
  void * item;
  struct list_node * next;
  pthread_rwlock_t next_lock;
} ;

struct list_head{
    int n_elements;
    pthread_rwlock_t n_elements_lock;
    int (*compareItemWithId) (void *, void *);
    void (*freeItem) (void *);
    pthread_rwlock_t first_node_lock;
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
  ListNode * prev, * aux;

  new_list_node = (ListNode *) malloc(sizeof(ListNode));
  new_list_node->item = item;

  pthread_rwlock_t lock;

  pthread_rwlock_wrlock(&list->n_elements_lock); // Lock para actualizar tamanho da list
  list->n_elements ++;
  pthread_rwlock_unlock(&list->n_elements_lock);


  lock = list->first_node_lock; // Lock para leitura do primeiro node
  pthread_rwlock_rdlock(&lock);
  aux = list->first_node;
  pthread_rwlock_unlock(&lock);

  // Verifica se deve introduzir no inicio da lista
  if(list->first_node == NULL || list->compareItemWithId(aux->item, item_id) != GREATER){

    lock = list->first_node_lock;
    pthread_rwlock_wrlock(&lock); // Lock para escrita do primeiro node
    new_list_node->next = list->first_node;
    list->first_node = new_list_node;
    pthread_rwlock_unlock(&lock);

    return;
  }

  do {
    prev = aux;

    lock = aux->next_lock;
    pthread_rwlock_rdlock(&lock); // Lock de leitura do node seguinte
    aux = aux->next;
    pthread_rwlock_unlock(&lock);

  } while(aux != NULL && list->compareItemWithId(aux->item, item_id) == SMALLER);

  lock = prev->next_lock;
  pthread_rwlock_wrlock(&lock); // Lock de escrita
  new_list_node->next = aux;
  prev->next = new_list_node;
  pthread_rwlock_unlock(&lock);
}

// Estas funções devolvem NULL se for usada a função defaultCompareItems
void * removeListItem(List * list, void * id){
  ListNode * aux;
  ListNode * prev = NULL;
  ListNode * prev_prev = NULL;
  void * item;
  int comparsion;

  pthread_rwlock_t lock;


  lock = list->first_node_lock; // Lock para leitura do primeiro node
  pthread_rwlock_rdlock(&lock);
  aux = list->first_node;
  pthread_rwlock_unlock(&lock);

  // O elemento encontra-se no inicio da lista
  if (aux != NULL && list->compareItemWithId(aux->item, id) == EQUAL){
    item = aux->item;

    lock = list->first_node_lock;
    pthread_rwlock_wrlock(&lock); // Lock para entrada escrita no primeiro node
    list->n_elements --;
    list->first_node = aux->next;
    pthread_rwlock_unlock(&lock);

    free(aux);

    // O utilizador é responsavel pelo free do item
    return item;
  }


  do {
    prev_prev = prev;
    prev = aux;

    lock = aux->next_lock;
    pthread_rwlock_rdlock(&lock); // Lock de leitura do node seguinte
    aux = aux->next;
    pthread_rwlock_unlock(&lock);
  } while(aux != NULL && (comparsion = list->compareItemWithId(aux->item, id)) == SMALLER);

  if (comparsion == EQUAL){
    item = aux->item;

    lock = list->n_elements_lock;
    pthread_rwlock_wrlock(&lock);
    list->n_elements --;
    pthread_rwlock_unlock(&lock);


    lock = prev_prev == NULL ? list->first_node_lock : prev_prev->next_lock; // Se prev_prev = NULL significa que se está a eliminar o primeiro elemento
    pthread_rwlock_wrlock(&lock); // Lock de escrita
    prev->next = aux->next;
    pthread_rwlock_unlock(&lock);

    free(aux);

    // O utilizador é responsavel pelo free do item
    return item;
  }

  return NULL;
}
void * findListItem(List * list, void * id){
  ListNode * aux;
  int comparsion = SMALLER;

  pthread_rwlock_t lock;

  lock = list->first_node_lock;
  pthread_rwlock_rdlock(&lock);
  aux = list->first_node;
  pthread_rwlock_unlock(&lock);


  // Avança enquanto aux for válido e o id do elemento actual for meno que id
  // (a lista está ordenada por ordem crescente de ids)
  while(aux != NULL && (comparsion = list->compareItemWithId(aux->item, id)) == SMALLER){
    lock = aux->next_lock;
    pthread_rwlock_rdlock(&lock);
    aux = aux->next;
    pthread_rwlock_unlock(&lock);
  }

  return comparsion == EQUAL ? aux->item : NULL;
}

ListNode * getFirstListNode(List * list){
  ListNode * node;

  pthread_rwlock_rdlock(&list->first_node_lock);
  node = list->first_node;
  pthread_rwlock_unlock(&list->first_node_lock);

  return node;
}
ListNode * getNextListNode(ListNode * list_node){
  ListNode * node;

  if (list_node == NULL) return NULL;

  pthread_rwlock_rdlock(&list_node->next_lock);
  node = list_node->next;
  pthread_rwlock_unlock(&list_node->next_lock);

  return node;
}
void * getListNodeItem(ListNode * list_node){
  return list_node == NULL ? NULL : list_node->item;
}

int getListSize(List * list){
  int n_elements;
  pthread_rwlock_rdlock(&list->n_elements_lock); //Lock de leitura
  n_elements = list->n_elements;
  pthread_rwlock_unlock(&list->n_elements_lock);
  return n_elements;
}
