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

// ----------------------------------------------
uint32_t gallery_add_photo(int peer_socket, char *file_name){ // TIPO 1
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
		novaimagem->type=size;
		buffer = serialize(novaimagem);
		close(picture);
		sendto(peer_socket, buffer,sizeof(PicInfo) , 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));
		// para o peer arranjar um vector com espaço pa isto
		
		
		novaimagem->type=1;
		novaimagem->file_name=*file_name;
		novaimagem->keyword=NULL;
		novaimagem->id_photo=getpid(); //random number mas que nao se repitA- NAO É SO ISTO FALTA QQR COISA
		buffer = serialize(novaimagem);
		sendto(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));
		
		free(buffer);		
		
		return (novaimagem->id_photo);
	}
}

// ----------------------------------------------
int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword) { // TIPO 2
	PicInfo * novosdados;
	struct sockaddr_in peer_socket;
	char * buffer;
	List * picList;

	novosdados->type=2;
	novosdados->keyword=keyword;
	novosdados->id_photo=id_photo;	
	buffer = serialize(novosdados);
	sendto(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));
	free(buffer);		
	
}

// ----------------------------------------------
int gallery_search_photo(int peer_socket, char * keyword, uint32_t ** id_photos) { // TIPO 3
	PicInfo * novosdados;
	struct sockaddr_in peer_socket;
	int i;
	int * buffer;

	if (peer_socket < 0 || * keyword== NULL || ** id_photos== NULL){
		return -1;
	}

	novosdados->type=3;
	novosdados->keyword=keyword;
	novosdados->id_photo=id_photos;	

	buffer = serialize(novosdados);
	sendto(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));

	isInterrupted = FALSE; 
	alarm(TIME_OUT);/// é isto???
	recvfrom(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&client_addr, (socklen_t *)&err);
	novosdados = deserialize(buffer);
	if (isInterrupted == TRUE){
		return -1;
	}

	
	buffer = (int*)calloc(novosdados->type, sizeof(uint32_t));
	for( i=0 ; i < novosdados->type; i++ ) {
		token = strtok(novosdados->id_photo, ',');
		while( token != NULL ) {
			buffer[i]= token;
			token = strtok(NULL, ',');
   		}
   	} 
   free(buffer);

   return(novosdados->type);
}

// ----------------------------------------------
int gallery_delete_photo(int peer_socket, uint32_t  id_photo) { // TIPO 4
	PicInfo * novosdados;
	struct sockaddr_in peer_socket;
	int i;
	int * buffer;

	if (peer_socket < 0 || id_photo == NULL){
		return -1;
	}

	novosdados->type=4;
	novosdados->id_photo=id_photo;	

	buffer = serialize(novosdados);
	sendto(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));

	isInterrupted = FALSE; 
	alarm(TIME_OUT);/// é isto???
	recvfrom(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&client_addr, (socklen_t *)&err);
	novosdados = deserialize(buffer);
	if (isInterrupted == TRUE){
		return -1;
	}

   free(buffer);

   return(novosdados->type);

}

// ----------------------------------------------
int gallery_get_photo_name(int peer_socket, uint32_t id_photo, char **photo_name) { // TIPO 5
	PicInfo * novosdados;
	struct sockaddr_in peer_socket;
	int *buffer;
	

	if (peer_socket < 0 || id_photo == NULL ){
		return -1;
	}

	novosdados->type=5;
	novosdados->id_photo=id_photo;	

	// nao faco ideia se e isto que se faz

	buffer = serialize(novosdados);
	sendto(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));

	isInterrupted = FALSE; 
	alarm(TIME_OUT);/// é isto???
	recvfrom(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&client_addr, (socklen_t *)&err);
	novosdados = deserialize(buffer);
	if (isInterrupted == TRUE){
		return -1;
	}

	**photo_name= (int*)calloc(novosdados->file_name, sizeof(char));

   free(buffer);

   return(novosdados->type);
}

int gallery_get_photo(int peer_socket, uint32_t id_photo, char *file_name) { // TIPO 6
	PicInfo * novosdados;
	struct sockaddr_in peer_socket;
	int * buffer;
	FILE *fp;
	List *novaimagem;

	if (peer_socket < 0 || id_photo == NULL){
		return -1;
	}

	novosdados->type=6;
	novosdados->id_photo=id_photo;	
	novosdados->file_name=file_name;

	buffer = serialize(novosdados);
	sendto(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&peer_socket, sizeof(peer_socket));

	isInterrupted = FALSE; 
	alarm(TIME_OUT);/// é isto???
	recvfrom(peer_socket, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&client_addr, (socklen_t *)&err);
	novosdados = deserialize(buffer);
	if (isInterrupted == TRUE){
		return -1;
	}
	insertList(novaimagem, novosdados); // nao ha de ser bem assim é suposto aqui fazer se o download

    sprintf(filename, "%c.%c", file_name, extensao); //onde é que arranjamos a extensao?
    fp = fopen(filename,"w");
	
   	free(buffer);


   return(novosdados->type);
}

