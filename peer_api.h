#ifndef PEER_API_H_INCLUDED
#define PEER_API_H_INCLUDED

#include "stdint.h"

#include "Estruturas/headed_list.h"

typedef struct photo{
	uint32_t id;
  char * file_name;
	int size;
	List * keywords;
} Photo;

void freePhoto(void * item);
int comparePhotoWithPhotoId(void * a, void * b);

int peer_add_photo_client(int fd, List * photos_list);

int peer_add_keyword(int fd, List * photos_list);
int peer_search_photo(int fd, List * photos_list);
int peer_delete_photo(int fd, List * photos_list);
int peer_get_photo_name(int fd, List * photos_list);
int peer_get_photo(int fd, List * photos_list);

#endif
