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

#define MAX_QUEUED_CONNECTIONS 128

#define TIMEOUT -1 //-1 //segundos

#define PEER_FOLDER_PREFIX "peer"

#define CHUNK_SIZE 512 // Nº de bytes da imagem lidos/escritos simultaneamente

#define UDP_RETRY_ATTEMPS 1

// Identificadores das mensagens trocadas
#define MSG_CLIENT_NEW_IMAGE 0
#define MSG_ADD_KEYWORD 1
#define MSG_SEARCH_PHOTO 2
#define MSG_DELETE_PHOTO 3
#define MSG_GET_PHOTO_NAME 4
#define MSG_GET_PHOTO 5

#define MSG_REPLICA_CLIENT_NEW_IMAGE 6
#define MSG_REPLICA_ADD_KEYWORD 7
#define MSG_REPLICA_DELETE_PHOTO 8

#define MSG_REPLICA_ALL 9

// Mensagens Peer <-> Gateway
#define MSG_GATEWAY_NEW_PEER_ID 10
#define MSG_GATEWAY_PEER_INFO 11
#define MSG_GATEWAY_PEER_DEATH 12
#define MSG_GATEWAY_IS_ALIVE 13

#endif
