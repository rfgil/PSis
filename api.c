#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "global.h"
#include "api.h"

int gallery_connect(char * host, in_port_t port){
	int fd_udp, fd_tcp, err;
	
	struct sockaddr_in gateway_addr;
	char * buffer;
	GatewayMsg * msg;

	// INICIALIZA SOCKET CLIENT UDP
	fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	checkError(fd_udp, "socket");
	
	// Define addr da gateway
	gateway_addr.sin_family = AF_INET;
	gateway_addr.sin_port = port;
	gateway_addr.sin_addr.s_addr = inet_ntoa(host);

	buffer = malloc(sizeof(GatewayMsg));

	// Envia e recebe mensagem da gateway
	sendto(fd_udp, buffer, 1, 0, (struct sockaddr *)&gateway_addr, sizeof(gateway_addr)); // Envia 1 byte qualquer
	recvfrom(fd_udp, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&gateway_addr, (socklen_t *)&err);
	
	msg = deserialize(buffer);
	
	if (msg->type != 0){
		// Abrir socket tcp
	}
	
	return NULL;
}
