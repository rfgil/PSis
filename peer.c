#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <stdio.h>

#include "global.h"
#include "peer_api.h"
#include "api.h"
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

			case MSG_REPLICA_ALL:
				printf("Pedido para replicar informação...\n");
				sendPhotoList(fd, photos_list);
				break;

			default: // Invalid message id -> termina a comunicação
				printf("Mensagem com ID desconhecido...\n");
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
	ListNode * current_node;

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
	current_node = getFirstListNode(client_threads_list);
	while(current_node != NULL){
		client_thread = getListNodeItem(current_node);
		current_node = getNextListNode(current_node);

		close(client_thread->fd);
		pthread_cancel(client_thread->thread);
		pthread_join(client_thread->thread, NULL);
	}

	// Liberta memória alocada para aa lista de threads abertos
	freeList(client_threads_list);
}


int openSockets(int * fd_udp, int * fd_tcp, in_port_t peer_port, struct in_addr peer_addr){
	// Abre sockets TCP e UDP
	*fd_udp = getBindedUDPSocket(peer_port, peer_addr);
	if (*fd_udp == ERROR) return ERROR;

	*fd_tcp = getBindedTCPSocket(peer_port, peer_addr);
	if (*fd_tcp == ERROR) return ERROR;

	return TRUE;
}

int downloadPeersData(char * gateway_host, in_port_t gateway_port){
	int fd, check;

	fd = gallery_connect(gateway_host, gateway_port);
  if (fd == ERROR) return ERROR;

	if ( (check = receivePhotoList(fd, &photos_list)) == TRUE){
		close(fd);
	}

	return check;
}

int main(int argc, char *argv[]){
	int fd_udp, fd_tcp;
	pthread_t gateway_thread;

	struct sockaddr_in gateway_sockaddr;
	in_port_t peer_port, gateway_client_port;
	struct in_addr peer_addr;

	// São necessários 4 argumentos: address e porto da gateway e do peer
	// argv[1] - gateway address
	// argv[2] - gateway client port
	// argv[3] - gateway peer port

	// argv[4] - peer address
	// argv[5] - peer port
	assert(argc == 6);

	// Inicializa addresses e ports em função dos argumentos
	assert(inet_aton(argv[1], &gateway_sockaddr.sin_addr) != 0);
	gateway_client_port	= htons(atoi(argv[2]));
	gateway_sockaddr.sin_port = htons(atoi(argv[3]));


	assert(inet_aton(argv[4], &peer_addr) != 0);
	peer_port = htons(atoi(argv[5]));

	// Abre sockets UDP e TCP para este peer
	if (openSockets(&fd_udp, &fd_tcp, peer_port, peer_addr) == ERROR) {
		perror(NULL);
		freeList(photos_list);
		return 1;
	}

	// Obtem PEER_ID
	if (serializeInteger(fd_udp, gateway_sockaddr, MSG_GATEWAY_NEW_PEER_ID) == ERROR ||
	    deserializeInteger(fd_udp, &PEER_ID) == ERROR){

		printf("Can't connect to gateway\n");
		close(fd_udp);
		close(fd_tcp);
		perror(NULL);
		return 1;
	}
	printf("Got id %d\n", PEER_ID);

	// Inicializa o vetor de photo_lists (com as fotos de outros peers já no sistema)
	if (downloadPeersData(argv[1], gateway_client_port) == ERROR) {
		close(fd_udp);
		close(fd_tcp);
		perror(NULL);
		return 1;
	}

	// Regista peer na gateway e obtem o seu ID
	if (registerAtGateway(fd_udp, gateway_sockaddr, peer_port, peer_addr) == ERROR) {
		close(fd_udp);
		close(fd_tcp);
		freeList(photos_list);
		printf("Error trying to regist peer at gateway\n");
		return ERROR;
	}
	printf("Peer registed with id %d\n", PEER_ID);

	setInterruptionHandler();

	// Abre thread responsável por manter comunicações com a gateway
	pthread_create(&gateway_thread, NULL, handleGateway, &fd_udp);

	// Aguarda por ligações de clientes
	waitClientConnection(fd_tcp);

	// Termina thread da gateway
	pthread_cancel(gateway_thread);
	pthread_join(gateway_thread, NULL);

	printf("Informando gateway do término deste peer...\n");
	unregisterAtGateway(fd_udp, gateway_sockaddr, PEER_ID);

	freeList(photos_list);

	// Fecha Sockets
	close(fd_udp);
	close(fd_tcp);

	return 0;
}
