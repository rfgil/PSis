#include <string.h>

#include <unistd.h>

#include "global.h"
#include "serializer.h"

// Criação de sockets
static int getBindedSocket(int socket_type, in_port_t port, struct in_addr addr){
	int fd;
	struct sockaddr_in socket_addr;

	// Abre socket TCP
	fd = socket(AF_INET, socket_type, 0);
	if (fd == -1) return ERROR;

	// Define propriedades da socket
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = port;
	socket_addr.sin_addr = addr;

	if (bind(fd, (struct sockaddr *) &socket_addr, sizeof(socket_addr)) == ERROR) return ERROR;

	// Faz bind e listen à socket criada
	return fd;
}
int getBindedUDPSocket(in_port_t port, struct in_addr addr){
	return getBindedSocket(SOCK_DGRAM, port, addr);
}
int getBindedTCPSocket(in_port_t port, struct in_addr addr){
	int fd;

	if ((fd = getBindedSocket(SOCK_STREAM, port, addr)) == ERROR) return ERROR;
	if (listen(fd, MAX_QUEUED_CONNECTIONS) == ERROR) return ERROR;

	return fd;
}


// Funções de leitura personalizadas
int TCPRead(int fd, void * buf, size_t nbytes, int timeout){
	fd_set rfds;
	struct timeval timer;

	int read_size;
	int offset = 0;

	do {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		timer.tv_sec = timeout;

		select(fd + 1, &rfds, NULL, NULL, timeout < 0 ? NULL : &timer);
		if (!FD_ISSET(fd, &rfds)) return ERROR; // Timeout ou interrupção

		read_size = read(fd, buf + offset, nbytes - offset);
		if(read_size <= 0) return ERROR; // A ligação foi termianda ou ocorreu um erro ao ler

		offset += read_size;

	} while(offset != nbytes);

	return TRUE;
}
int UDPRead(int fd, void * buf, size_t nbytes, int timeout){
	fd_set rfds;
	struct timeval timer;

	int n_tries = 0;
	int check;

	do {
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		timer.tv_sec = timeout;

		select(fd + 1, &rfds, NULL, NULL, &timer);
		if (FD_ISSET(fd, &rfds)){
			check = recv(fd, buf, nbytes, 0);
			if (check == -1) return ERROR;

			if (check == nbytes){
				// Foi recebido a quantidade de bytes prevista
				return TRUE;
			} else {
				return FALSE;
			}
		}

		n_tries ++;
	} while(n_tries < UDP_RETRY_ATTEMPS);

	return ERROR;
}

// ---------------- PEER / GATEWAY messages ----------------
// Peer pretende registar-se na gateway (Peer -> Gateway)
// Gateway fornece um peer ao client (Gateway -> Client)
int   serializePeerInfo(int fd, struct sockaddr_in destination, in_port_t   peer_port, struct in_addr   peer_addr){
	char * buffer;
	int msg_id, offset;

  offset = 0;
  msg_id = MSG_GATEWAY_PEER_INFO;

	buffer = (char *) malloc(sizeof(int) + sizeof(in_port_t) + sizeof(struct in_addr));

  memcpy(buffer + offset, &msg_id, sizeof(int));
  offset += sizeof(int);
  memcpy(buffer + offset, &peer_port, sizeof(in_port_t));
  offset += sizeof(in_port_t);
  memcpy(buffer + offset, &peer_addr, sizeof(struct in_addr));
  offset += sizeof(struct in_addr);

  return sendto(fd, buffer, offset, 0, (struct sockaddr *) &destination, sizeof(destination));
}
int deserializePeerInfo(char * buffer, in_port_t * peer_port, struct in_addr * peer_addr){
	int msg_id, offset;

	memcpy(&msg_id, buffer, sizeof(int));
	if (msg_id != MSG_GATEWAY_PEER_INFO) {free(buffer); return ERROR;}
	// Lança erro se a mensagem não tinha o id previsto

	offset = sizeof(int);
  memcpy(peer_port, buffer + offset, sizeof(in_port_t));
  offset += sizeof(in_port_t);
  memcpy(peer_addr, buffer + offset, sizeof(struct in_addr));

	return TRUE;
}

int serializePeerDeath(int fd, struct sockaddr_in destination, int peer_id){
	int msg_id = MSG_GATEWAY_PEER_DEATH;
	char * buffer = malloc(2*sizeof(int));

	memcpy(buffer, &msg_id, sizeof(int));
	memcpy(buffer + sizeof(int), &peer_id, sizeof(int));

	return sendto(fd, buffer, 2*sizeof(int), 0, (struct sockaddr *) &destination, sizeof(destination));
}
int deserializePeerDeath(char * buffer, int * peer_id){
	int msg_id;

	memcpy(&msg_id, buffer, sizeof(int));
	if (msg_id != MSG_GATEWAY_PEER_DEATH) {free(buffer); return ERROR;}
	// Lança erro se a mensagem não tinha o id previsto

	memcpy(peer_id, buffer + sizeof(int), sizeof(int));
	return TRUE;
}

// Utilizado para respostas a requests
int   serializeInteger(int fd, struct sockaddr_in destination, int integer){
	return sendto(fd, &integer, sizeof(int), 0, (struct sockaddr *) &destination, sizeof(destination));
}
int deserializeInteger(int fd, int * integer){
	int check;
	char * buffer = malloc(sizeof(int));

	check = UDPRead(fd, buffer, sizeof(int), TIMEOUT);
	if (check == ERROR || check == FALSE) return check;

	memcpy(integer, buffer, sizeof(int));
	free(buffer);

	return TRUE;
}



// ---------------- CLIENT / GATEWAY messages ----------------
