#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define TRUE 1
#define FALSE 0
#define ERROR -1

#define GATEWAY_PORT_CLIENTS 3000
#define GATEWAY_PORT_PEERS 4000

#define MAX_MSG_LEN 100

#define TIMEOUT 5 //segundos

typedef struct network_msg{
	int msg_id;
	void * info;
} NetworkMsg;

// Mensagens enviadas do cliente para o peer
#define MSG_NEW_IMAGE 0
#define MSG_ADD_KEYWORD 1
#define MSG_SEARCH_PHOTO 2
#define MSG_DELETE_PHOTO 3
#define MSG_GET_PHOTO_NAME 4
#define MSG_GET_PHOTO 5



typedef struct message_gw{
	int type;
	char address[20];
	int port;
} GatewayMsg;

typedef struct image_info{
	uint32_t id;
	char * name;
	long size;
} ImageInfo;

GatewayMsg * deserializeGatewayMsg(char * buffer){
	GatewayMsg * msg;

	msg = (GatewayMsg *) malloc(sizeof(GatewayMsg));
	assert(msg != NULL);

	memcpy(msg, buffer, sizeof(GatewayMsg));
	return msg;
}

char * serializeGatewayMsg(GatewayMsg * msg){
	char * buffer;

	buffer = (char *) malloc(sizeof(GatewayMsg));
	assert(buffer != NULL);

	memcpy(buffer, msg, sizeof(GatewayMsg));
	return buffer;
}

unsigned char * serializeImageInfo(ImageInfo * img, int * size){
	unsigned char * buffer;
	int aux;

	*size = sizeof(uint32_t) + sizeof(long) + (strlen(img->name) + 1)*sizeof(char);

	buffer = (unsigned char *) malloc(*size);
	aux = 0;

	memcpy(buffer + aux, &img->id, sizeof(uint32_t));
	aux += sizeof(uint32_t);

	memcpy(buffer + aux, &img->size, sizeof(long));
	aux += sizeof(long);

	memcpy(buffer + aux, img->name, (strlen(img->name)+1)*sizeof(char));
	aux += (strlen(img->name)+1) * sizeof(char);

	assert(*size != aux); // Confirmação de que foi tudo serializado corretamente
	return buffer;
}

#endif
