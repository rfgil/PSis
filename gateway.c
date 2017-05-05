#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/un.h>

#include <signal.h>
#include <pthread.h>

//#include <bits/sigaction.h> //???

#include "global.h"
#include "Estruturas/list.h"

//WIRESHARK

/*
struct sigaction {
   void     (*sa_handler)(int);
   void     (*sa_sigaction)(int, siginfo_t *, void *);
   sigset_t   sa_mask;
   int        sa_flags;
   void     (*sa_restorer)(void);
};
*/

List * peerList;

int isInterrupted = FALSE;

typedef struct _peerInfo{
	char address[20];
	int port;
} PeerInfo;


void interruptionHandler(){
	isInterrupted = TRUE;
}
	
void * waitPeer(void * arg){
	struct sockaddr_in peer_addr;
	char * buffer;
	GatewayMsg * msg;
	
	List * aux;
	PeerInfo * peer;
	
	int isNew, size_addr, sock_fd;
	
	
	buffer = malloc(MAX_MSG_LEN*sizeof(char));
	
	// O fd da socket é enviado como argumento para o thread
	sock_fd = *((int*)arg);
	
	while(1){
		isNew = TRUE; // Assume-se que se receberá um novo peer por defeito
		
		size_addr = sizeof(peer_addr);
		recvfrom(sock_fd, buffer, MAX_MSG_LEN, 0, (struct sockaddr *)&peer_addr, (socklen_t *)&size_addr);
		
		// Verifica se houve alguma interrupção
		if (isInterrupted){
			close(sock_fd);
			free(buffer);
			
			pthread_exit(0);
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


void * waitClient(void * arg){
	struct sockaddr_in client_addr;
	char * in_buffer, * out_buffer;
	GatewayMsg * msg;
	List * currentPeer;
	int size_addr, sock_fd;
	
		
	in_buffer = malloc(MAX_MSG_LEN*sizeof(char));
	
	// O fd da socket é enviado como argumento para o thread
	sock_fd = *((int*)arg);
	
	// Apontador para a próximo peer da lista a ser enviado
	currentPeer = NULL;
	
	while(1){ // O cliente procura um servidor disponivel
		// Aguarda mensagem de um cliente
		size_addr = sizeof(client_addr);
		recvfrom(sock_fd, in_buffer, MAX_MSG_LEN, 0, (struct sockaddr *)&client_addr, (socklen_t *)&size_addr);
		
		// Verifica se houve alguma interrupção (recvfrom é interrompido nesse caso)
		if (isInterrupted){
			close(sock_fd);
			free(in_buffer);
			
			pthread_exit(0);
		}
		
		//msg = deserialize(in_buffer); // A mensagem enviada pelo cliente é irrelevante
		msg = malloc(sizeof(GatewayMsg));

		
		if (peerList != NULL) { // Confirma se a lista de servidores não está vazia
			if (currentPeer == NULL){ // Permite regressar ao inicio da lista de servidores (implementa o round-robin)
				currentPeer = peerList;
			}
			
			// Define mensagem a enviar
			msg->type = TRUE; // Significa que há um peer disponivel
			strcpy(msg->address, ((PeerInfo *) currentPeer->item)->address);
			msg->port = ((PeerInfo *)currentPeer->item)->port;
			
			currentPeer = currentPeer->next; // Avança na lista para o próximo peer
			
		} else { // não há nenhum peer disponivel
			
			msg->type = FALSE; // Significa que NÃO há um peer disponivel
		}
		
		out_buffer = serialize(msg);
		sendto(sock_fd, out_buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
		free(out_buffer);
		
		free(msg);
	}
}


int main(){
	struct sigaction sa;
	struct sockaddr_in local_addr;
	int sock_fd_client, sock_fd_peer, err;
	
	pthread_t client_thread, peer_thread;
	
			
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
	
	// Define handle para interrupções
	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = interruptionHandler;
	sigaction(SIGINT, &sa, NULL);
	
	// Cria threads
	pthread_create(&client_thread, NULL, waitClient, &sock_fd_client);
	pthread_create(&peer_thread, NULL, waitPeer, &sock_fd_peer);
	
	// Espera que todos os threads terminem
	pthread_join(client_thread, NULL);
	pthread_join(peer_thread, NULL);

	return 0;
}
