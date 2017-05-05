#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>

void checkError(int var, char * description){
	if(var == -1) {
		perror(description);
		exit(-1);
	}
}

int main(){
	int sock_fd_clientudp, sock_fd_clienttcp, err;

	//INICIALIZA SOCKET CLIENT UDP
	sock_fd_clientudp = socket(AF_INET, SOCK_DGRAM, 0);
	checkError(sock_fd_clientudp, "socket");	

	//INICIALIZA SOCKET CLIENT TCP
	sock_fd_clienttcp = socket(AF_INET, SOCK_DGRAM, 0); 
	checkError(sock_fd_client, "socket");
	

	// Bind socket para clients UDP
	local_addr.sin_family = AF_INET; // Definições da socket UDP
	local_addr.sin_port= htons(GATEWAY_PORT_CLIENTS);
	local_addr.sin_addr.s_addr= INADDR_ANY;	
	err = bind(sock_fd_clientudp, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");

	// Bind socket para clientes TCP
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(GATEWAY_PORT_PEERS);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	err = bind(sock_fd_clienttcp, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");


	//ENVIA PARA GATEWAY
	sendto(sock_fd_clientudp, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&local_addr, sizeof(local_addr));
	
	//RECEBE DA GATEWAY
	recv(sock_fd_clienttcp, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&local_addr, sizeof(local_addr));
	
//    CONNECT
//    SEND/RECV

	//Close
	close(sock_fd_clientudp);
	close(sock_fd_clienttcp);


	return 0;
}