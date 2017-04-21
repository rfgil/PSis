#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>

#include "global.h"
#include "Estruturas/list.h"

//WIRESHARK

void checkError(int var, char * description){
	if(var == -1) {
		perror(description);
		exit(-1);
	}
}

char * serialize(GatewayMsg * msg){
	char * buffer;
	buffer = malloc(sizeof(GatewayMsg));
	memcpy(buffer, msg, sizeof(GatewayMsg));
	return buffer;
}

GatewayMsg * deserialize(char * buffer){
	GatewayMsg * msg;
	msg = malloc(sizeof(GatewayMsg));
	memcpy(msg, buffer, sizeof(GatewayMsg));
	return msg;
}

void handleServidor(GatewayMsg * msg, List ** serverList){
	List * aux;
	GatewayMsg * listedServer;
	
	// Confirma se este servidor já se encontra na lista
	aux = *serverList;
	while(aux != NULL){
		listedServer = ((GatewayMsg *)aux->item);
		if (strcmp(msg->address, listedServer->address) == 0 && msg->port == listedServer->port){
			return;
		}		
		aux = aux->next;
	}
	
	// Adiciona servidor à lista
	*serverList = insertList(*serverList, msg);
}

int main(){
	struct sockaddr_in local_addr, client_addr;
	int sock_fd, err, size_addr;
	char * buffer;
	GatewayMsg * msg;
	
	List * serverList;
	List * currentServer;
	
	
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_DGRAM -> UDP | SOCK_STREAM -> TCP
	checkError(sock_fd, "socket");
	
	// Definições da socket UDP
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(GATEWAY_PORT);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	
	err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");
	
	serverList = newList();
	currentServer = NULL;
	
	while(1){
		size_addr = sizeof(client_addr);
		recvfrom(sock_fd, buffer, MAX_MSG_LEN, 0, (struct sockaddr *)&client_addr, (socklen_t *)&size_addr);
		
		msg = deserialize(buffer);
		
		if (msg->type == 0){        // Cliente
			if (serverList != NULL) {
				if (currentServer == NULL){ // Permite regressar ao inicio da lista de servidores
					currentServer = serverList;
				}
				
				buffer = serialize(currentServer->item);
				sendto(sock_fd, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
				free(buffer);
				
				currentServer = currentServer->next;
			}
			
		} else if (msg->type == 1){ // Servidor
			handleServidor(msg, &serverList);
		}
		
		free(msg);
	}
	
	close(sock_fd);

	return 0;
}
