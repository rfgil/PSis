#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>

#include "api.h"

void checkError(int var, char * description){
	if(var == -1) {
		perror(description);
		exit(-1);
	}
}

int main(int argc, char *argv[]){
	int fd_udp, fd_tcp, err;
	struct sockaddr_in gateway_addr;
	char * buffer;
	GatewayMsg * msg;

	// INICIALIZA SOCKET CLIENT UDP
	fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	checkError(fd_udp, "socket");
	
		
	local_addr.sin_family = AF_INET; // Definições da socket UDP
	local_addr.sin_port = htons(GATEWAY_PORT_CLIENTS);
	local_addr.sin_addr.s_addr = INADDR_ANY;

	buffer = malloc(sizeof(char));

	// ENVIA PARA GATEWAY
	sendto(fd_udp, buffer, 1, 0, (struct sockaddr *)&gateway_addr, sizeof(local_addr));
	

	
	
	

	

	//INICIALIZA SOCKET CLIENT TCP
	sock_fd_clienttcp = socket(AF_INET, SOCK_DGRAM, 0); 
	checkError(sock_fd_client, "socket");

	// Bind socket para clientes TCP
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(GATEWAY_PORT_PEERS);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	err = bind(sock_fd_clienttcp, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");



	
	//RECEBE DA GATEWAY
	recv(sock_fd_clienttcp, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&local_addr, sizeof(local_addr));
	
//    CONNECT
//    SEND/RECV

	//Close
	close(sock_fd_clientudp);
	close(sock_fd_clienttcp);


	return 0;
}
