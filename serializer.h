#ifndef SERIALIZER_H_INCLUDED
#define SERIALIZER_H_INCLUDED

#include <arpa/inet.h>

#include "global.h"

int getBindedUDPSocket(in_port_t port, struct in_addr addr);
int getBindedTCPSocket(in_port_t port, struct in_addr addr);

int TCPRead(int fd, void * buf, size_t nbytes, int timeout);
int UDPRead(int fd, void * buf, size_t nbytes, int timeout);

int    serializePeerInfo(int fd, struct sockaddr_in destination, in_port_t   peer_port, struct in_addr   peer_addr);
int deserializePeerInfo(char * buffer, in_port_t * peer_port, struct in_addr * peer_addr);

int serializePeerDeath(int fd, struct sockaddr_in destination, int peer_id);
int deserializePeerDeath(char * buffer, int * peer_id);

int serializeInteger(int fd, struct sockaddr_in destination, int integer);
int deserializeInteger(int fd, int * integer);

#endif
