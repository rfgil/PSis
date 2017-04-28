#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <signal.h>

#include "global.h"
#include "Estruturas/list.h"

//WIRESHARK

List * peerList;
List * currentPeer;
int isInterrupted = FALSE;

typedef struct _peerInfo{
	char address[20];
	int port;
} PeerInfo;


void interruptionHandler(){
	isInterrupted = TRUE;
}
	
void checkError(int var, char * description){
	if(var == -1) {
		perror(description);
		exit(-1);
	}
}

char * serialize(GatewayMsg * msg){
	char * buffer;
	buffer = malloc(sizeof(GatewayMsg));
	memcpy(buffer, msg, sizeof(GatewayMsg));
	return buffer;
}

GatewayMsg * deserialize(char * buffer){
	GatewayMsg * msg;
	msg = malloc(sizeof(GatewayMsg));
	memcpy(msg, buffer, sizeof(GatewayMsg));
	return msg;
}


void waitPeer(void * arg){
	struct sockaddr_in peer_addr;
	char * buffer;
	GatewayMsg * msg;
	
	List * aux;
	PeerInfo * peer;
	
	int isNew, size_addr, sock_fd;
	
	// O fd da socket é enviado como argumento para o thread
	sock_fd = *((int*)arg);
	
	while(1){
		isNew = TRUE; // Assume-se que se receberá um novo peer por defeito
		
		size_addr = sizeof(peer_addr);
		recvfrom(sock_fd, buffer, MAX_MSG_LEN, 0, (struct sockaddr *)&peer_addr, (socklen_t *)&size_addr);
		
		// Verifica se houve alguma interrupção
		if (isInterrupted){
			close(sock_fd);
			exit(0);
		}
		
		msg = deserialize(buffer);	
		
		// Confirma se este servidor já se encontra na lista
		aux = peerList;
		while(aux != NULL){
			peer = ((PeerInfo *)aux->item);
			if (strcmp(msg->address, peer->address) == 0 && msg->port == peer->port){
				isNew = FALSE;
				break;
			}		
			aux = aux->next;
		}
		
		if (isNew){
			// Cria novo peer
			peer = malloc(sizeof(PeerInfo));
			//checkError(peer, "malloc");
			
			strcpy(peer->address, msg->address);
			peer->port = msg->port;
			
			// Insere novo peer na lista
			peerList = insertList(peerList, peer);
		}
		
		free(msg);
	}
}

void waitClient(void * arg){
	
}


void handleServidor(GatewayMsg * msg, List ** serverList){
	//List * aux;
	//GatewayMsg * listedServer;
	

}

int main(){
	struct sockaddr_in local_addr;
	int sock_fd_client, sock_fd_peer, err;
	
	pthread_t client_thread, peer_thread;
	
	struct sigaction sigHandler;
		
	sock_fd_client = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_DGRAM -> UDP | SOCK_STREAM -> TCP
	sock_fd_peer = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_DGRAM -> UDP | SOCK_STREAM -> TCP
	checkError(sock_fd_client, "socket");
	checkError(sock_fd_peer, "socket");
	
	// Bind socket para clients
	local_addr.sin_family = AF_INET; // Definições da socket UDP
	local_addr.sin_port= htons(GATEWAY_PORT_CLIENTS);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	
	err = bind(sock_fd_client, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");
	
	// Bind socket para peers
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(GATEWAY_PORT_PEERS);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	
	err = bind(sock_fd_peer, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");
	
	// Iniciliza lista
	peerList = newList();
	currentPeer = NULL;
	
	// Define handle para interrupções
	sigHandler.sa_handler = interruptionHandler;
	sigaction(SIGINT, &sigHandler, NULL);
	
	// Cria threads
	pthread_create(&client_thread, NULL, waitClient, &sock_fd_client);
	pthread_create(&peer_thread, NULL, waitPeer, &sock_fd_peer);
	
	// Espera que todos os threads terminem
	pthread_join(client_thread, NULL);
	pthread_join(peer_thread, NULL);
	
	return 0;
	
	/*
	while(1){
		
		
		msg = deserialize(buffer);
		
		if (msg->type == 0){        // Cliente
			if (serverList != NULL) {
				if (currentServer == NULL){ // Permite regressar ao inicio da lista de servidores
					currentServer = serverList;
				}
				
				buffer = serialize(currentServer->item);
				sendto(sock_fd, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
				free(buffer);
				
				currentServer = currentServer->next;
			} else {
				// A mensagem recebida é do tipo 0 (foi um cliente que a enviou para a gateway).
				// Se não houver nenhum servidor na lista pode ser enviada a mesma mensagem com type=0,
				// que significa (para o cliente) que não há um servidor disponivel
				//
				sendto(sock_fd, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
			}
			
		} else if (msg->type == 1){ // Servidor
			handleServidor(msg, &serverList);
		}
		
		free(msg);
	}
	
	close(sock_fd);
	*/

	return 0;
}
