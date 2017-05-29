#include <sys/time.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>

#include "Estruturas/headed_list.h"
#include "global.h"
#include "api.h"

static int identify_photo_protocol(int peer_socket, uint32_t id_photo, int msg_id){
	int check, size;

	// Informação enviada:
	// int - identificador da mensagem
	// int - id da imagem
	// Aguarda confirmação de que a imagem existe (int)

	// Envia o identificador do tipo de mensagem que será enviado
	check = send(peer_socket, &msg_id, sizeof(int), 0);
	if (check == -1) return ERROR;

	// Envia o id da foto em causa
	check = send(peer_socket, &id_photo, sizeof(uint32_t), 0);
	if (check == -1) return ERROR;

	// Recebe o tamanho em bytes da foto identificada
	check = myRead(peer_socket, &size, sizeof(int), TIMEOUT);
	if (check == ERROR) return ERROR;

	return size;
}

int gallery_connect(char * host, in_port_t port){
	int badjoras = 0;
	/*GatewayMsg * msg;
	struct sockaddr_in addr;

	fd_set rfds;
	struct timeval timeout;

	int fd_udp, fd_tcp, err;
	char * buffer;

	buffer = malloc(sizeof(GatewayMsg));

	// INICIALIZA SOCKET CLIENT UDP
	fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	assert(fd_udp != -1);

	// Define addr da gateway
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	err = inet_aton(host, &addr.sin_addr);
	assert(err != 0);

	// Envia mensagem para a gateway (1 byte qualquer)
	err = sendto(fd_udp, buffer, 1, 0, (struct sockaddr *) &addr, sizeof(addr));
	assert(err != -1);

	// TIME_OUT
	FD_ZERO(&rfds);
	FD_SET(fd_udp, &rfds);
	timeout.tv_sec = TIMEOUT;

	err = select(fd_udp + 1, &rfds, (fd_set *) NULL, (fd_set *) NULL, &timeout);
	assert(err != -1);

	if (!FD_ISSET(fd_udp, &rfds)){
		return -1; // Timeout (Assum que não é possivel aceder à gateway)
	}

	// Recebe resposta da gateway
	recvfrom(fd_udp, buffer, sizeof(GatewayMsg), 0, (struct sockaddr *)&addr, (socklen_t *)&err);
	msg = deserializeGatewayMsg(buffer);

	if (msg->type == 0){
		return 0; // No peer is available
	}

	// Abrir socket tcp
	fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	assert(fd_tcp != -1);

	// Define addr do peer
	addr.sin_family = AF_INET;
	addr.sin_port= htons(msg->port);
	err = inet_aton(msg->address, &addr.sin_addr);
	assert(err != -0);

	err = connect(fd_tcp, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
	assert(err != -1);

	return fd_tcp;
	*/return badjoras;
}

uint32_t gallery_add_photo(int peer_socket, char * file_name){
	FILE * image_file = NULL;
	unsigned char buffer[CHUNK_SIZE];
	int size, check, nbytes, id;

	if (peer_socket < 0 || file_name == NULL){
		return 0;
	}

	// Informação enviada:
	// int - identificador da mensagem
	// int - tamanho da string file_name
	// char - file_name
	// int - tamanho da imagem
	// char - imagem

	image_file = fopen(file_name, "rb");
	if (image_file == NULL)	return 0; // Nesta função o erro é 0

	// Envia identificador do tipo de mensagem que será enviado
	check = send(peer_socket, MSG_CLIENT_NEW_IMAGE, sizeof(int), 0);
	if (check == -1) return 0;

	// Envia tamanho da string file_name
	size = strlen(file_name);
	check = send(peer_socket, &size, sizeof(int), 0);
	if (check == -1) return 0;

	// Envia file_name
	check = send(peer_socket, file_name, size*sizeof(char), 0);
	if (check == -1) return 0;

	// Envia tamanho da imagem (em bytes)
	fseek(image_file, 0, SEEK_END); // Permite a leitura do tamanho da imagem
	size = ftell(image_file);
	check = send(peer_socket, &size, sizeof(int), 0);
	if (check == -1) return 0;

	// Envia a imagem
	fseek(image_file, 0, SEEK_SET); // Regressa ao inicio do ficheiro
	while ( (nbytes = fread(buffer, sizeof(char), CHUNK_SIZE, image_file) ) > 0) {
		check = send(peer_socket, buffer, nbytes, 0);
		if (check == -1) return 0;
	}

	// Recebe id da imagem
	check = myRead(peer_socket, &id, sizeof(int), TIMEOUT);
	if (check == -1) return 0;

	return id;
}

int gallery_add_keyword(int peer_socket, uint32_t id_photo, char * keyword) {
	int check, size;

	size = identify_photo_protocol(peer_socket, id_photo, MSG_ADD_KEYWORD);
	if (size == 0) return FALSE; // A foto não existe
	if (size < 0) return ERROR;

	size = strlen(keyword);
	check = send(peer_socket, &size, sizeof(int), 0);
	if (check == -1) return ERROR;

	check = send(peer_socket, keyword, size*sizeof(char), 0);
	if (check == -1) return ERROR;

	return TRUE;
}

int gallery_search_photo(int peer_socket, char * keyword, uint32_t ** id_photos) {
	int check, msg_id, i, size;
	int photos_count;

	//uint32_t * received_id;

	if (peer_socket < 0 || keyword == NULL){
		return -1;
	}

	// Envia o identificador do tipo de mensagem que será enviado
	msg_id = MSG_SEARCH_PHOTO;
	check = send(peer_socket, &msg_id, sizeof(int), 0);
	if (check == -1) return ERROR;

	// Envia tamanho da keyword
	size = strlen(keyword);
	check = send(peer_socket, &msg_id, sizeof(int), 0);
	if (check == -1) return ERROR;

	// Envia a keyword
	check = send(peer_socket, keyword, size*sizeof(int), 0);
	if (check == -1) return ERROR;

	// Recebe numero de ids que vai receber
	check = myRead(peer_socket, &photos_count, sizeof(int), TIMEOUT);
	if (check == -1) return ERROR;

	// Aloca espaço para o vetor de ids
	*id_photos = (uint32_t *) calloc(photos_count, sizeof(uint32_t));

	// Recebe os vários ids
	for(i = 0; i<photos_count; i++){
		check = myRead(peer_socket, &(*id_photos[i]), sizeof(uint32_t), TIMEOUT);
		if (check == -1) return ERROR;
	}

	return photos_count; // A variavel i irá conter o size da lista
}

int gallery_delete_photo(int peer_socket, uint32_t  id_photo) {
	int size;

	size = identify_photo_protocol(peer_socket, id_photo, MSG_DELETE_PHOTO);
	if (size == 0) return FALSE; // A foto não existe
	if (size < 0) return ERROR;

	return TRUE;
}

int gallery_get_photo_name(int peer_socket, uint32_t id_photo, char ** photo_name) {
	int check, size;

	check = identify_photo_protocol(peer_socket, id_photo, MSG_GET_PHOTO_NAME);
	if (check == 0) return FALSE;
	if (check < 0) return ERROR;

	// Recebe tamanho da string file_name
	check = myRead(peer_socket, &size, sizeof(int), TIMEOUT);
	if (check == -1) return ERROR;

	// Aloca espaço necessário para armazenar a string
	*photo_name = (char *) malloc((size + 1)*sizeof(char));
	*photo_name[size] = '\0';

	check = myRead(peer_socket, *photo_name, size*sizeof(char), TIMEOUT);
	if (check == -1) return ERROR;

	return TRUE;
}

int gallery_get_photo(int peer_socket, uint32_t id_photo, char *file_name) {
	int check, size, read_chunck_size;
	char buffer[CHUNK_SIZE];
	FILE * file;

	size = identify_photo_protocol(peer_socket, id_photo, MSG_GET_PHOTO);
	if (size == 0) return FALSE;
	if (size < 0) return ERROR;

	file = fopen(file_name, "wb");

	while(size > 0){
		read_chunck_size = size > CHUNK_SIZE ? CHUNK_SIZE : size;

		check = myRead(peer_socket, &buffer, read_chunck_size, TIMEOUT);
		if (check == -1) return ERROR;

		fwrite(buffer, sizeof(char), read_chunck_size, file);

		size -= read_chunck_size;
	}

	fclose(file);

	return TRUE;
}
