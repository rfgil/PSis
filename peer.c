#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <stdio.h>

#include "global.h"
#include "peer_api.h"

#define MAX_QUEUED_CONNECTIONS 128

List * photos_list;
List * client_threads_list;

in_port_t PEER_PORT;
struct in_addr PEER_ADDR;

int isInterrupted = FALSE;

void interruptionHandler(){
	isInterrupted = TRUE;
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

int connectToGateway(){
	return TRUE;
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
	}

	return NULL;
}

void waitClientConnection(){
	int fd, new_fd, check;
	struct sockaddr_in addr;
	ClientThread * client_thread;

	// Abre socket TCP
	fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(fd != -1);

	// Define propriedades da socket
	addr.sin_family = AF_INET;
	addr.sin_port = PEER_PORT;
	addr.sin_addr = PEER_ADDR;

	// Faz bind e listen à socket criada
	check = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
	if (check == -1) { perror(NULL); close(fd); return;}

	check = listen(fd, MAX_QUEUED_CONNECTIONS) != -1;
	if (check == -1) { perror(NULL); close(fd); return;}

	// Cria lista que armazena todos os threads criados
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

	// Fecha socket
	close(fd);
	printf("Ligação terminada!!!\n");

	// Espera que todos os threads terminem
	startListIteration(client_threads_list);
	while((client_thread = (ClientThread *) getListNextItem(client_threads_list) )!= NULL){
		close(client_thread->fd);
		pthread_cancel(client_thread->thread);
		printf("forced to close\n");
	}

	// Liberta memória alocada para a lista
	freeList(client_threads_list);
}

int main(int argc, char *argv[]){
	struct sigaction sa;

	assert(argc == 3); // São necessários 2 e apenas 2 argumentos (address e porto do servidor)
	assert(inet_aton(argv[1], &PEER_ADDR) != 0); // Garante que a address introduzida é válida
	PEER_PORT = htons(atoi(argv[2]));

	// Conecta o servider à gateway
	assert( connectToGateway() != ERROR );

	// Define handler para interrupções
	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = interruptionHandler;
	sigaction(SIGINT, &sa, NULL);

	photos_list = newList(comparePhotoWithPhotoId, freePhoto);
	waitClientConnection();
	freeList(photos_list);

	return 0;
}
