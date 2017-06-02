#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <stdio.h>

#include "global.h"
#include "peer_api.h"
#include "serializer.h"

List * photos_list;
List * client_threads_list;

int isInterrupted = FALSE;
void interruptionHandler(){
	isInterrupted = TRUE;
}
void setInterruptionHandler(){
	struct sigaction sa;

	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = interruptionHandler;
	sigaction(SIGINT, &sa, NULL);
}

typedef struct client_thread {
	pthread_t thread;
	int fd;
} ClientThread;

int compareThread(void * a, void * b){
	pthread_t thread_a = *(pthread_t *)a;
	pthread_t thread_b = *(pthread_t *)b;

	if (pthread_equal(thread_a, thread_b)){
		return EQUAL;
	} else {
		return thread_a > thread_b ? GREATER : SMALLER;
	}
}

static int myRead(int fd, void * buf, size_t nbytes){
	int read_size;
	int offset = 0;

	do {
		read_size = read(fd, buf + offset, nbytes - offset);
		if (isInterrupted) return FALSE;
		if(read_size <= 0) return ERROR; // A ligação foi termianda ou ocorreu um erro ao ler

		offset += read_size;

	} while(offset != nbytes);

	return TRUE;
}

void * handleGateway(void * arg){
	int size, msg_id;
	struct sockaddr_in caller_dest;

	int fd = *(int *)arg;

	while(!isInterrupted){
		size =  sizeof(caller_dest);
		recvfrom(fd, &msg_id, sizeof(int), 0, (struct sockaddr *) &caller_dest, (socklen_t *) &size); // Sem timeout
		if (isInterrupted) break;

		if (msg_id == MSG_GATEWAY_IS_ALIVE){
			// Responde com o numero de ligações actual (que corresponde ao numero de threads de clientes abertos)
			size = getListSize(client_threads_list);
			sendto(fd, &size, sizeof(int), 0, (struct sockaddr *) &caller_dest, sizeof(caller_dest));

			printf("Sending alive state to gateway! Handling %d Clients right now\n", size);
		}
		// Não é necessário serializer uma vez que só é trocado um inteiro
	}

	return NULL;
}

void * handleClient(void * item){
	int fd = *(int *) item;
	int msg_id;
	int check = TRUE;

	printf("Nova ligação\n");

	// O servidor fica indefinadmente à espera de interação por parte do cliente
	while(myRead(fd, &msg_id, sizeof(int)) == TRUE && check != ERROR){
		switch (msg_id) {
			case MSG_CLIENT_NEW_IMAGE:
				printf("Pedido para adicionar foto...\n");
				check = peer_add_photo_client(fd, photos_list);
				if (check == ERROR) perror(NULL);
				break;

			case MSG_ADD_KEYWORD:
			printf("Pedido para adicionar keyword...\n");
				check = peer_add_keyword(fd, photos_list);
				if (check == ERROR) perror(NULL);
				break;

			case MSG_SEARCH_PHOTO:
				printf("Pedido para procurar foto...\n");
				check = peer_search_photo(fd, photos_list);
				if (check == ERROR) perror(NULL);
				break;

			case MSG_DELETE_PHOTO:
				printf("Pedido para remover foto...\n");
				check = peer_delete_photo(fd, photos_list);
				if (check == ERROR) perror(NULL);
				break;

			case MSG_GET_PHOTO_NAME:
				printf("Pedido para obter nome da foto...\n");
				check = peer_get_photo_name(fd, photos_list);
				if (check == ERROR) perror(NULL);
				break;

			case MSG_GET_PHOTO:
				printf("Pedido para obter foto...\n");
				check = peer_get_photo(fd, photos_list);
				if (check == ERROR) perror(NULL);
				break;

			default: // Invalid message id -> termina a comunicação
				printf("Mensagem com ID desconhecido...\n");
				check = ERROR;
				break;
		}
	}

	// Fecha a socket
	close(fd);
	printf("Ligação terminada!\n");

	if(!isInterrupted){
		// Caso não tenha ocorrido uma interrupção, remove o thread da lista
		pthread_t self = pthread_self();
		ClientThread * client_thread = removeListItem(client_threads_list, &self);
		if (client_thread == NULL) return NULL;

		free(client_thread);
		pthread_detach(self); // Elimina os resources alocados pelo thred permitindo que não seja necessário fazer join
	}

	return NULL;
}

void waitClientConnection(int fd){
	int new_fd;
	ClientThread * client_thread;

	photos_list = newList(comparePhotoWithPhotoId, freePhoto);
	client_threads_list = newList(compareThread, free);

	// Aguarda por novas ligações de clientes
	while(!isInterrupted){
		new_fd = accept(fd, NULL, NULL);
		if (isInterrupted) break;
		if (new_fd == -1) { perror(NULL); close(fd); return;}; // Continua à espera de novas liações em caso de erro

		// Inicia novo thread para lidar com o cliente
		client_thread = (ClientThread *) malloc(sizeof(ClientThread));
		client_thread->fd = new_fd;
		pthread_create(&client_thread->thread, NULL, handleClient, &client_thread->fd); // Mudar o endereço de new_fd para cada thread

		// Insere ID do thread na lista
		insertListItem(client_threads_list, client_thread, &client_thread->thread);
	}

	// Força todos os threads a terminar
	startListIteration(client_threads_list);
	while((client_thread = (ClientThread *) getListNextItem(client_threads_list) )!= NULL){
		close(client_thread->fd);
		pthread_cancel(client_thread->thread);
		pthread_join(client_thread->thread, NULL);
	}

	// Liberta memória alocada para as listas
	freeList(photos_list);
	freeList(client_threads_list);
}

int getSocketsFromArguments(char *argv[], int * fd_udp, int * fd_tcp, struct sockaddr_in * gateway_addr){
	in_port_t peer_port;
	struct in_addr peer_addr;

	// Obtem Address da socket da gateway
	gateway_addr->sin_family = AF_INET;
	assert(inet_aton(argv[1], &gateway_addr->sin_addr) != 0); // Garante que a address introduzida é válida
	gateway_addr->sin_port =  htons(atoi(argv[2]));

	// Obtem address e port do peer
	assert(inet_aton(argv[3], &peer_addr) != 0); // Garante que a address introduzida é válida
	peer_port = htons(atoi(argv[4]));

	// Abre sockets TCP e UDP
	*fd_udp = getBindedUDPSocket(peer_port, peer_addr);
	if (*fd_udp == ERROR) return ERROR;

	*fd_tcp = getBindedTCPSocket(peer_port, peer_addr);
	if (*fd_tcp == ERROR) return ERROR;

	// Regista peer na gateway e ontem o seu ID
	PEER_ID = registerAtGateway(*fd_udp, *gateway_addr, peer_port, peer_addr);
	if (PEER_ID == ERROR) {
		// Fecha as sockets
		close(*fd_udp);
		close(*fd_tcp);
		printf("Error trying to regist peer at gateway\n");
		return ERROR;
	}

	printf("Peer registed with id %d\n", PEER_ID);

	return TRUE;
}

int main(int argc, char *argv[]){
	int fd_udp, fd_tcp;
	struct sockaddr_in gateway_addr;
	pthread_t gateway_thread;

	// São necessários 4 argumentos: address e porto da gateway e do peer
	assert(argc == 5);
	if ( getSocketsFromArguments(argv, &fd_udp, &fd_tcp, &gateway_addr) == ERROR) {perror(NULL); return 1;}

	setInterruptionHandler();

	// Abre thread responsável por manter comunicações com a gateway
	pthread_create(&gateway_thread, NULL, handleGateway, &fd_udp);

	// Aguarda por ligações de clientes
	waitClientConnection(fd_tcp);

	// Termina thread da gateway
	pthread_cancel(gateway_thread);
	pthread_join(gateway_thread, NULL);

	printf("Informando gateway do término deste peer...\n");
	unregisterAtGateway(fd_udp, gateway_addr, PEER_ID);

	// Fecha Sockets
	close(fd_udp);
	close(fd_tcp);

	return 0;
}
