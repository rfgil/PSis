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
	int fd_udp, fd_tcp, err, peer;
	
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
	
	peer = connect(fd_tcp, msg->address, sizeof(msg->address);
	
	
	return fd_tcp;
}

uint32_t gallery_add_photo(int peer_socket, char *file_name){ // TIPO 0
	PicInfo * novaimagem;
	struct sockaddr_in peer_socket;
	int size;
	FILE *picture;
	
	if (peer_socket < 0 || *file_name == NULL){
		return 0;
	}
	

	else{	
		picture = fopen(file_name);
		fseek(picture, 0, SEEK_END);
		size = ftell(picture);
		fseek(picture, 0, SEEK_SET);
		buffer = serialize(size);
		close(picture);
		sendto(peer_socket, buffer, 1, 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));
		// para o peer arranjar um vector com espaço pa isto
		
		
		novaimagem->type=0;
		novaimagem->file_name=*file_name;
		novaimagem->keyword=NULL;
		novaimagem->id_photo=getpid(); //random number mas que nao se repita	
		buffer = serialize(novaimagem);
		sendto(peer_socket, buffer, 1, 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));
		
		free(buffer);		
		
		return (novaimagem->id_photo);
	}
}


int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword) { // TIPO 1
	PicInfo * novakey;
	struct sockaddr_in peer_socket;
	char * buffer;
	List * picList;
	

	else{
		recvfrom(peer_socket, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&gateway_addr, (socklen_t *)&err);

		novaimagem->keyword=keyword;
		novaimagem->id_photo=id_photo;	
		buffer = serialize(novaimagem);
		sendto(peer_socket, buffer, 1, 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));
		free(buffer);		
	}

}
