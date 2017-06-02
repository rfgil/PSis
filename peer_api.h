#ifndef PEER_API_H_INCLUDED
#define PEER_API_H_INCLUDED

#include <stdint.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "Estruturas/headed_list.h"

int PEER_ID;

void freePhoto(void * item);
int comparePhotoWithPhotoId(void * a, void * b);

int registerAtGateway(int fd, struct sockaddr_in gateway_addr, in_port_t peer_port, struct in_addr peer_addr);
int unregisterAtGateway(int fd, struct sockaddr_in gateway_addr, int peer_id);

int peer_add_photo_client(int fd, List * photos_list);

int peer_add_keyword(int fd, List * photos_list);
int peer_search_photo(int fd, List * photos_list);
int peer_delete_photo(int fd, List * photos_list);
int peer_get_photo_name(int fd, List * photos_list);
int peer_get_photo(int fd, List * photos_list);

int sendPhotoList(int fd, List * photos_list);
int receivePhotoList(int fd, List ** photo_list);

#endif
