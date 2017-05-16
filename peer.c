#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>

#include "sock_dg.h"

#define TIME_OUT 10

int isInterrupted = FALSE;

void interruptionHandler(){
	isInterrupted = TRUE;
}
void alarmHandler(){
	isInterrupted  = TRUE;
}

void * wait(void * arg){ // tentativa manhosa
	struct sockaddr_in peer_addr;
	char * buffer;
	GatewayMsg * msg;
		
	int size_addr, fd_tcp; // nao deveria receber a fd_tcp??
	
	
	buffer = malloc(MAX_MSG_LEN*sizeof(char));
	
		size_addr = sizeof(peer_addr);
		recvfrom(fd_tcp, buffer, MAX_MSG_LEN, 0, (struct sockaddr *)&peer_addr, (socklen_t *)&size_addr);	
		// Verifica se houve alguma interrupção
		if (isInterrupted){
			close(fd_tcp);
			free(buffer);		
			pthread_exit(0);
		}

		/// o send é quando e do que?
		
		msg = deserialize(buffer);	
		free(msg);
	}
}


void informGateway(){
	int fd;
	struct sockaddr_in local_addr;
	GatewayMsg msg;
	char * buffer;
	
	// Abre discr	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	checkError(fd, "socket");
	
	// Define socket UDP
	local_addr.sin_family = AF_INET; // Definições da socket UDP
	local_addr.sin_port= htons(GATEWAY_PORT_CLIENTS);
	local_addr.sin_addr.s_addr= INADDR_ANY;		

	// Define a mensagem a enviar
	msg.type = 0; //????
	msg.port = local_addr.sin_port; // nao tenho certezas disto
	msg.address = local_addr.sin_addr.s_addr; //

	buffer = serialize(* msg);

	sendto(fd_udp, buffer, 1, 0, (struct sockaddr *)&gateway_addr, sizeof(gateway_addr)); // Envia 1 byte qualquer
	
}

int main(){

	int fd_tcp, err;
	struct sockaddr_in local_addr;
	GatewayMsg * msg;
	char * buffer;
	struct sigaction sa1, sa2;
	pthread_t thread;


	informGateway();

	//INICIALIZA SOCKET PEER TCP
	fd_tcp = socket(AF_INET, SOCK_STREAM, 0); 
	checkError(fd_tcp, "socket");

	
	// Bind socket TCP
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(GATEWAY_PORT_PEERS);
	local_addr.sin_addr.s_addr= INADDR_ANY;	

	err = bind(fd_tcp, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");

	// Define handle para interrupções
	memset (&sa1, 0, sizeof(sa1));
	sa1.sa_handler = alarmHandler;
	sigaction(SIGALRM, &sa1, NULL);
	memset (&sa2, 0, sizeof(sa2));
	sa2.sa_handler = interruptionHandler;
	sigaction(SIGINT, &sa2, NULL);

	//TCP
		//   Listen
		isInterrupted = FALSE; 
		alarm(TIME_OUT);
		recvfrom(fd_tdp, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&gateway_addr, (socklen_t *)&err);
		if (isInterrupted == TRUE){
			return -1;
		} //NAO SEI SE É SUPOSTO RETORNAR TB -1

		//   Accept -> aqui supostamente temos que verificar que conectou ao client nao sei como

	// Cria threads
	pthread_create(&thread, NULL, wait, &fd_tcp);	
	
	// Espera que todos os threads terminem
	pthread_join(thread, NULL);


	return 0;
}
