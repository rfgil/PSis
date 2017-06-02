#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <time.h>

#include <unistd.h>
#include <stdio.h>

#include <signal.h>
#include <pthread.h>


#include "global.h"
#include "Estruturas/headed_list.h"
#include "serializer.h"

// Interrupções
int isInterrupted = FALSE	;
void interruptionHandler(){
	isInterrupted = TRUE;
}
void setInterruptionHandler(){
	struct sigaction sa;

	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = interruptionHandler;
	sigaction(SIGINT, &sa, NULL);
}

// Variáveis globais
List * peer_list;
int CURRENT_PEER_ID = 0;

typedef struct peer_info{
	int id;
	int n_connections;
	//time_t last_msg;
	struct in_addr addr;
	in_port_t tcp_port;
	in_port_t udp_port;
} PeerInfo;

int comparePeerWithPeerId(void * peer, void * id){
	if (((PeerInfo *)peer)->id == *(int *)id){
		return EQUAL;
	}
	return ((PeerInfo *)peer)->id > *(int *)id ? GREATER : SMALLER;
}

int isPeerAlive(int fd, PeerInfo * peer){
	struct sockaddr_in addr;
	int msg_id = MSG_GATEWAY_IS_ALIVE;

	int n_connections;

	// Define propriedades da socket
	addr.sin_family = AF_INET;
	addr.sin_port = peer->udp_port;
	addr.sin_addr = peer->addr;

	printf("Checking if selected peer %d is alive...\n", peer->id);

	sendto(fd, &msg_id, sizeof(int), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr));

	if (UDPRead(fd, &n_connections, sizeof(int), TIMEOUT) == TRUE){
		peer->n_connections = n_connections + 1; // Nº de conexões atual + conexão que receberá já de seguida por ter sido selecionado
		return TRUE;
	}

	printf("Peer doesn't answer... Removing from list!\n");
	removeListItem(peer_list, &peer->id);

	return FALSE;
}

PeerInfo * getAvailablePeer(int fd){
	ListNode * current_node;
	PeerInfo * best_peer;
	PeerInfo * current_peer;

	do {
		current_node = getFirstListNode(peer_list);
		best_peer = getListNodeItem(current_node); // best_peer começa por ser o primeiro elemento da lista

		current_node = getNextListNode(current_node); // current_node o segundo elemento da lista
		while(current_node != NULL){
			current_peer = getListNodeItem(current_node);
			current_node = getNextListNode(current_node); // Iteração pela lista com a variavel current_node

			if (current_peer->n_connections < best_peer->n_connections){
				best_peer = current_peer;
			}
		}

	} while(best_peer != NULL && isPeerAlive(fd, best_peer) != TRUE);

	 return best_peer; // Pode ser NULL, caso não existam servidores na lista
}

void * handleClients(void * arg){
	int fd, check;
	struct sockaddr_in client_addr;

	PeerInfo * peer;
	char buffer[CHUNK_SIZE];
	socklen_t size;

	fd = *(int *)arg;

	while(!isInterrupted){
		size = sizeof(client_addr);
		check = recvfrom(fd, buffer, CHUNK_SIZE, 0, (struct sockaddr *) &client_addr, &size);
		if (isInterrupted) return NULL;
		if (check == -1) {perror(NULL); continue;}

		printf("Novo pedido de cliente...\n");

		// Não é importante o que os clientes enviam
		// (eles apenas contactam a gateway para obter um servidor)

		// Obtem peer disponivel e serializa a sua informação
		peer = getAvailablePeer(fd);
		if (peer != NULL){
			serializePeerInfo(fd, client_addr, peer->id, peer->tcp_port, peer->addr);
			printf("Indicando peer com id %d em %s@%d\n", peer->id, inet_ntoa(peer->addr), (int) ntohs(peer->tcp_port));

		} else {
			serializeInteger(fd, client_addr, FALSE);
			printf("Não há peers disponiveis\n");
		}
	}

	return NULL;
}

void * handlePeersUDP(void * arg){
	int fd, msg_id, peer_id;
	struct sockaddr_in peer_addr;

	PeerInfo * peer;
	char buffer[CHUNK_SIZE];
	socklen_t size;

	fd = *(int *)arg;

	while(!isInterrupted){
		// Recebe mensagem
		size = sizeof(peer_addr);
		if ( recvfrom(fd, buffer, CHUNK_SIZE, 0, (struct sockaddr *) &peer_addr, &size) == ERROR) { perror(NULL); close(fd); return NULL;};

		// Obtem identificador da mensagem
		memcpy(&msg_id, buffer, sizeof(int));

		switch(msg_id){
			case MSG_GATEWAY_NEW_PEER_ID:
				printf("Pedido de novo ID para peer\n");
				serializeInteger(fd, peer_addr, CURRENT_PEER_ID++);
				break;

			case MSG_GATEWAY_PEER_INFO: // Novo Peer!
				printf("Novo peer...\n");

				// Aloca nova estrutura que representa o servidor
				peer = (PeerInfo *) malloc(sizeof(PeerInfo));

				peer->n_connections = 0;
				peer->udp_port = peer_addr.sin_port; // Neste momento a gateway foi contactada via socket UDP do peer

				if (deserializePeerInfo(buffer, &peer->id, &peer->tcp_port, &peer->addr) == TRUE){
					insertListItem(peer_list, peer, &peer->id); // Insere na lista de servidores

					serializeInteger(fd, peer_addr, TRUE); // Confirma que o peer foi inserido
				} else {
					serializeInteger(fd, peer_addr, ERROR); // Informa o peer que ocorreu um erro na sua inserção
				}
				break;

			case MSG_GATEWAY_PEER_DEATH: // Eliminar Peer
				printf("Remover Peer...\n");

				deserializePeerDeath(buffer, &peer_id);
				removeListItem(peer_list, &peer_id);
				// Não é necessário responder
		}
	}

	return NULL;
}

void sendPeerList(int fd){
	ListNode * current_node;
	PeerInfo * peer;
	int size;

	size  = getListSize(peer_list);
	if (send(fd, &size, sizeof(int), 0) == ERROR) return;

	current_node = getFirstListNode(peer_list);
	while(current_node != NULL){
		peer = getListNodeItem(current_node);
		current_node = getNextListNode(current_node);

		if ( send(fd, &peer->tcp_port, sizeof(in_port_t), 0)  == ERROR) return;
		if ( send(fd, &peer->addr, sizeof(struct in_addr), 0) == ERROR) return;
	}
}

void handlePeersTCP(int fd){
	int new_fd, check;

	check = listen(fd, MAX_QUEUED_CONNECTIONS) != -1;
	if (check == -1) { perror(NULL); close(fd); return;}

	while(!isInterrupted){
		new_fd = accept(fd, NULL, NULL);
		if (isInterrupted) break;
		if (new_fd == -1) { perror(NULL); close(fd); return;}; // Continua à espera de novas liações em caso de erro

		sendPeerList(new_fd);
		close(new_fd);
	}

	close(fd);
}

int main(int argc, char *argv[]){
	pthread_t clients_thread, peers_thread;

	struct in_addr gateway_addr;
	int fd_peers_udp, fd_peers_tcp, fd_clients;

	assert(argc > 3);
	assert(inet_aton(argv[1], &gateway_addr) != 0); // Garante que a address introduzida é válida

	// Abre todas as sockets necessárias
	fd_clients = getBindedUDPSocket(htons(atoi(argv[2])), gateway_addr);
	if (fd_clients == ERROR) {perror(NULL); return 1;}

	fd_peers_udp = getBindedUDPSocket(htons(atoi(argv[3])), gateway_addr);
	if (fd_peers_udp == ERROR) {perror(NULL); return 1;}

	fd_peers_tcp = getBindedTCPSocket(htons(atoi(argv[3])), gateway_addr);
	if (fd_peers_tcp == ERROR) {perror(NULL); return 1;}

	setInterruptionHandler();

	// Cria lista de peers
	peer_list = newList(comparePeerWithPeerId, free);

	pthread_create(&clients_thread, NULL, handleClients, &fd_clients);
	pthread_create(&peers_thread, NULL, handlePeersUDP, &fd_peers_udp);

	handlePeersTCP(fd_peers_tcp);

	// Termina threads
	pthread_cancel(clients_thread);
	pthread_cancel(peers_thread);

	pthread_join(clients_thread, NULL);
	pthread_join(peers_thread, NULL);

	freeList(peer_list);
}
