#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define ERROR -1

#define GATEWAY_PORT_CLIENTS 3000
#define GATEWAY_PORT_PEERS 4000

#define MAX_MSG_LEN 100

#define TIMEOUT 5 //segundos

#define PEER_FOLDER_PREFIX "peer/"

#define TIME_OUT 10

#define CHUNK_SIZE 512 // Nº de bytes da imagem lidos/escritos simultaneamente

// Identificadores das mensagens trocadas
#define MSG_CLIENT_NEW_IMAGE 0
#define MSG_PEER_NEW_IMAGE 1
#define MSG_ADD_KEYWORD 2
#define MSG_SEARCH_PHOTO 3
#define MSG_DELETE_PHOTO 4
#define MSG_GET_PHOTO_NAME 5
#define MSG_GET_PHOTO 6

int myRead(int fd, void * buf, size_t nbytes, int timeout){
	fd_set rfds;
	struct timeval timer;

	int read_size;
	int offset = 0;

	do {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		timer.tv_sec = timeout;

		select(fd + 1, &rfds, NULL, NULL, timeout < 0 ? NULL : &timer);
		if (!FD_ISSET(fd, &rfds)) return ERROR; // Timeout ou interrupção

		read_size = read(fd, buf + offset, nbytes - offset);
		if(read_size <= 0) return ERROR; // A ligação foi termianda ou ocorreu um erro ao ler

		offset += read_size;

	} while(offset != nbytes);

	return TRUE;
}

#endif
