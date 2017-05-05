#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>

#include "sock_dg.h"

int main(){

	int sock_fd_peerudp, sock_fd_peertcp, err;

	//INICIALIZA SOCKET PEER UDP	
	sock_fd_peerudp = socket(AF_INET, SOCK_DGRAM, 0);
	checkError(sock_fd_peerudp, "socket");
		

	//INICIALIZA SOCKET PEER TCP
	sock_fd_peertcp = socket(AF_INET, SOCK_DGRAM, 0); 
	checkError(sock_fd_peertcp, "socket");

	
	// Bind socket para peer UDP
	local_addr.sin_family = AF_INET; // Definições da socket UDP
	local_addr.sin_port= htons(GATEWAY_PORT_CLIENTS);
	local_addr.sin_addr.s_addr= INADDR_ANY;	
	err = bind(sock_fd_peerudp, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");

	// Bind socket para peer TCP
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(GATEWAY_PORT_PEERS);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	err = bind(sock_fd_peertcp, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");

//UDP
	//ENVIA PARA GATEWAY
	sendto(sock_fd_peerudp, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&local_addr, sizeof(local_addr));
	//RETURN E SUPOSTAMENTE ACABA AQUI A UDP

	
//TCP
//   Listen
//   Accept
//   Recv/send



	//Close
	close(sock_fd_peerudp);

	close(sock_fd_peertcp); //la so diz pthread_exit


	return 0;
}