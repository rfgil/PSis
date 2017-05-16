#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "global.h"
#include "api.h"

#define TIME_OUT 10

int hasRead;

int isInterrupted = FALSE;

void alarmHandler(){
	isInterrupted  = TRUE;
}

int gallery_connect(char * host, in_port_t port){
	int fd_udp, fd_tcp, err;
	
	struct sockaddr_in gateway_addr;
	char * buffer;
	GatewayMsg * msg;
	
	struct timeval stTimeOut;
	struct sigaction sa;
	


	

	// INICIALIZA SOCKET CLIENT UDP
	fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	checkError(fd_udp, "socket");
	
	// Define addr da gateway
	gateway_addr.sin_family = AF_INET;
	gateway_addr.sin_port = port;
	gateway_addr.sin_addr.s_addr = inet_ntoa(host);
	
	// Define função de handle para o alarm (SIGALARM)
	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = alarmHandler;
	sigaction(SIGALRM, &sa, NULL);

	buffer = malloc(sizeof(GatewayMsg));

	// Envia e recebe mensagem da gateway
	sendto(fd_udp, buffer, 1, 0, (struct sockaddr *)&gateway_addr, sizeof(gateway_addr)); // Envia 1 byte qualquer
	isInterrupted = FALSE; 
	alarm(TIME_OUT);
	recvfrom(fd_udp, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&gateway_addr, (socklen_t *)&err);
	if (isInterrupted == TRUE){
		return -1;
	}
	
	msg = deserialize(buffer);
	
	if (msg->type == 0){
		// No peer is available
		return 0;
	}
	
	// Abrir socket tcp
	fd_tcp = socket(AF_INET, SOCK_STREAM, 0); 
	checkError(fd_tcp, "socket");

	// Define addr do peer
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(msg->port);
	local_addr.sin_addr.s_addr= inet_ntoa(msg->address);
	
	
	return fd_tcp;
}
