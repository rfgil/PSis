#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>

#include "sock_dg.h"

void informGateway(){
	int fd;
	GatewayMsg msg;
	
	// Abre discr	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	checkError(fd, "socket");
	
	// Bind socket para peer UDP
	local_addr.sin_family = AF_INET; // Definições da socket UDP
	local_addr.sin_port= htons(GATEWAY_PORT_CLIENTS);
	local_addr.sin_addr.s_addr= INADDR_ANY;		

	msg.type = 
	msg.

	sendto(fd_udp, buffer, 1, 0, (struct sockaddr *)&gateway_addr, sizeof(gateway_addr)); // Envia 1 byte qualquer
	
}

int main(){

	int sock_fd_peerudp, sock_fd_peertcp, err;


		

	//INICIALIZA SOCKET PEER TCP
	sock_fd_peertcp = socket(AF_INET, SOCK_STREAM, 0); 
	checkError(sock_fd_peertcp, "socket");

	
	// Bind socket para peer UDP
	local_addr.sin_family = AF_INET; // Definições da socket UDP
	local_addr.sin_port= htons(GATEWAY_PORT_PEERS);
	local_addr.sin_addr.s_addr= INADDR_ANY;	

//TCP
//   Listen
//   Accept
//   Recv/send



	//Close
	close(sock_fd_peerudp);

	close(sock_fd_peertcp); //la so diz pthread_exit


	return 0;
}
