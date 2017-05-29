#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include <unistd.h>
#include <stdio.h>

#include "global.h"
#include "peer_api.h"

#define MAX_QUEUED_CONNECTIONS 128

List * photos_list;
List * client_threads;

in_port_t PEER_PORT;
struct in_addr PEER_ADDR;

int isInterrupted = FALSE;

int compareThread(void * a, void * b){
	if (*(pthread_t *)a == *(pthread_t *)b) {
		return EQUAL;
	}
	return *(pthread_t *)a > *(pthread_t *)b ? GREATER : SMALLER; // Não
}

int connectToGateway(){
	return TRUE;
}

void * handleClient(void * item){
	int fd = *(int *) item;
	int msg_id;
	int check = TRUE;

	// O servidor fica indefinadmente à espera de interação por parte do cliente
	while(myRead(fd, &msg_id, sizeof(int), -1) == TRUE && check != ERROR){
		switch (msg_id) {
			case MSG_CLIENT_NEW_IMAGE:
				check = peer_add_photo_client(fd, photos_list);
				break;

			case MSG_ADD_KEYWORD:
				check = peer_add_keyword(fd, photos_list);
				break;

			case MSG_SEARCH_PHOTO:
				check = peer_search_photo(fd, photos_list);
				break;

			case MSG_GET_PHOTO_NAME:
				check = peer_get_photo_name(fd, photos_list);
				break;

			case MSG_GET_PHOTO:
				check = peer_get_photo(fd, photos_list);
				break;

			default: // Invalid message id -> termina a comunicação
				check = ERROR;
				break;
		}
	}

	if(!isInterrupted){
		// Caso não tenha ocorrido uma interrupção, remove o thread da lista
		pthread_t self = pthread_self();
		removeListItem(client_threads, &self);
	}

	// Fecha a socket
	close(fd);

	return NULL;
}

void waitClientConnection(){
	int fd, new_fd;
	struct sockaddr_in addr;
	pthread_t * thread;

	// Abre socket TCP
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(fd != -1);

	// Define propriedades da socket
	addr.sin_family = AF_INET;
	addr.sin_port = PEER_PORT;
	addr.sin_addr = PEER_ADDR;

	// Faz bind e listen à socket criada
	assert( bind(fd, (struct sockaddr *) &addr, sizeof(addr)) != -1);
	assert( listen(fd, MAX_QUEUED_CONNECTIONS) != -1);

	// Cria lista que armazena todos os threads criados
	client_threads = newList(compareThread, NULL);

	// Aguarda por novas ligações de clientes
	while(!isInterrupted){
		new_fd = accept(fd, NULL, NULL);
		if (new_fd == -1) continue; // Continua à espera de novas liações em caso de erro

		// Insere ID do thread na lista
		thread = (pthread_t *) malloc(sizeof(pthread_t));
		insertListItem(client_threads, thread, thread); // Thread é simultaneamente o item a inserir e o id

		// Inicia novo thread para lidar com o cliente
		pthread_create(thread, NULL, handleClient, &new_fd);
	}


	// Espera que todos os threads terminem
	startListIteration(client_threads);
	while((thread = (pthread_t *) getListNextItem(client_threads) )!= NULL){
		pthread_join(*thread, NULL);
	}

	// Liberta memória alocada para a lista
	freeList(client_threads);
}

int main(int argc, char *argv[]){
	assert(argc == 3); // São necessários 2 e apenas 2 argumentos (address e porto do servidor)
	assert(inet_aton(argv[1], &PEER_ADDR) != 0); // Garante que a address introduzida é válida
	PEER_PORT = htons(atoi(argv[2]));

	// Conecta o servider à gateway
	assert( connectToGateway() != ERROR );

	photos_list = newList(comparePhotoWithPhotoId, freePhoto);
	waitClientConnection();
	freeList(photos_list);

	return 0;
}

/*
int main2(){
	int fd_tcp, err , client;
	struct sockaddr_in local_addr;
	GatewayMsg * msg;
	char * buffer;
	struct sigaction sa1, sa2;
	pthread_t thread;
	// Define handle para interrupções
	memset (&sa1, 0, sizeof(sa1));
	sa1.sa_handler = alarmHandler;
	sigaction(SIGALRM, &sa1, NULL);
	memset (&sa2, 0, sizeof(sa2));
	sa2.sa_handler = interruptionHandler;
	sigaction(SIGINT, &sa2, NULL);

	//INICIALIZA SOCKET PEER TCP
	fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	checkError(fd_tcp, "socket");

	// Bind socket TCP
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(GATEWAY_PORT_PEERS);
	local_addr.sin_addr.s_addr= INADDR_ANY;

	err = bind(fd_tcp, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");

	// Define a mensagem e envia Para Gateway
	msg.type = 0; //
	msg.port = htons(GATEWAY_PORT_PEERS);
	msg.address = (struct sockaddr *)&local_addr;
	buffer = serialize(* msg);
	informGateway(*buffer);

	//TCP
		//   Listen
		isInterrupted = FALSE;
		alarm(TIME_OUT);
		recvfrom(fd_tcp, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&gateway_addr, (socklen_t *)&err);
		if (isInterrupted == TRUE){
			return -1;
		} //NAO SEI SE É SUPOSTO RETORNAR TB -1

		//  Accept -> aqui supostamente temos que verificar que conectou ao client nao sei como
	client = accept(fd_tcp, local_addr, sizeof(local_addr)));
	if (client > 0) {
		// Cria threads
		pthread_create(&thread, NULL, handle_client, &fd_tcp);
		// Espera que todos os threads terminem
		pthread_join(thread, NULL);
		}
		return 0;
}



//------------------------------------------------------------------------------------------


void interruptionHandler(){
	isInterrupted = TRUE;
}
void alarmHandler(){
	isInterrupted  = TRUE;
}

void informGateway(*buffer){
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

	// Envia para a gateway o PORT da TCP
	sendto(fd_udp, buffer, 1, 0, (struct sockaddr *)&gateway_addr, sizeof(gateway_addr)); // Envia 1 byte qualquer

}

void handle_client(*arg) {
	struct sockaddr_in client_addr;
	char * buffer;
	int sock_fd, tamanho, tipo, pic_id;
	List * picList;
	List * aux;
	PicInfo * novosdados;
	PicInfo * picture;
	PicInfo * pictureaux;

	buffer = malloc(MAX_MSG_LEN*sizeof(char));

	// O fd da socket é enviado como argumento para o thread
	sock_fd = *((int*)arg);

	// Iniciliza lista -> Não sei se será aqui (será uma diferente para cada peer)
	picList = newList();

	// Define mensagem a enviar
	buffer = serialize(picList);

	sendto(sock_fd, buffer, 1, 0, (struct sockaddr *)&client_addr, sizeof(sock_fd));

	isInterrupted = FALSE;
	alarm(TIME_OUT);
	recvfrom(fd_udp, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&client_addr, (socklen_t *)&err);
	if (isInterrupted == TRUE){
		return -1;
	}

	novosdados = deserialize(buffer);
	tipo = novosdados->type;

	if (tipo >10) {
		tamanho = tipo;

			recvfrom(fd_udp, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&client_addr, (socklen_t *)&err);
			novosdados = deserialize(buffer);
			if (novosdados->type == 1)
			{
				insertList(picList, novosdados);
			}
	}


	if (tipo == 2) {
		aux = picList;
		while(aux != NULL){
			picture = ((PicInfo *)aux->item);
			if (strcmp(novosdados->id_photo, picture->id_photo) == 1) {
				strcat (picture->keyword,',');
				strcat (picture->keyword,novosdados->keyword);
				break;
			}
			aux = aux->next;
		}
	}

	if (tipo == 3) {
		aux = picList;
		pictureaux->id_photo=novosdados->id_photo;
		while(aux != NULL){
			picture = ((PicInfo *)aux->item);
			token = strtok(picture->keyword, ',');
   			while( token != NULL ) {
   				if (strcmp(novosdados->keyword, token) == 1) {
   					pictureaux->type= 1 + pictureaux->type;

   					strcat (pictureaux->id_photo,',');
   					strcat (pictureaux->id_photo,novosdados->(* id_photo));

 // melhorar isto
   					break;
   				}
   			  token = strtok(NULL, ',');
   			}
   		aux = aux->next;
		}

		buffer = serialize(pictureaux);
		sendto(sock_fd, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&client_addr, sizeof(sock_fd));
	}

	if (tipo == 4 || tipo == 5 || tipo ==6) {
		novosdados->type=0;
		if (picList != NULL && novosdados->type==0) {
			aux = picList->next;
			while(aux!= NULL && novosdados->type==0){
				picture = ((PicInfo *)aux->item);

   				if(strcmp(novosdados->id_photo, picture->id_photo) == 1) {
   			 		novosdados->type=1;
   			 		if (Tipo==4) {
   			 			picList->next = aux->next;
   			 		}
   			 		if (Tipo==4) {
   			 			novosdados->file_name = picture->file_name;
   			 		}
   			 		if (Tipo==6){
   			 			picture = fopen(picture->file_name);
						fseek(picture, 0, SEEK_END);
						size = ftell(picture);
						fseek(picture, 0, SEEK_SET);
						novosdados->type=size;
   			 		}
   				}
 			picList = picList->next;
   			aux = aux->next
   			}
		}
		buffer = serialize(novosdados);
		sendto(sock_fd, buffer, sizeof(PicInfo), 0, (struct sockaddr *)&client_addr, sizeof(sock_fd));
		if(Tipo==6) {
			close(picture);
		}
	}

	free(buffer);

}
*/
//-----------------------------------------------------------------
