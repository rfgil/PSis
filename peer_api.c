#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>

#include "peer_api.h"
#include "global.h"
#include "serializer.h"

typedef struct photo{
	uint32_t id;
  char * file_name;
	int size;
	List * keywords;
} Photo;


static char * getFileExtension(char * buffer){
	int i;

	for (i = (strlen(buffer) - 1); i >= 0; i--){
		if (buffer[i] == '.') return &buffer[i];
	}

	return buffer;
}

static char * getPhotoFileName(Photo * photo, char * buffer){
	if (buffer == NULL){
		buffer = malloc(strlen(PEER_FOLDER_PREFIX) + 30);
		// id_max=2^32 -> 10 caracteres, Nenhuma extensão de imagens ocupa mais de 6 caracteres, +1 para \0
	}

	sprintf(buffer, "%s(%d)%d%s", PEER_FOLDER_PREFIX, PEER_ID, photo->id, getFileExtension(photo->file_name));
	return buffer;
}

static int readToFile(int fd, int size, char * output_file){
	FILE * file;
	char buffer[CHUNK_SIZE];
	int check, current_chunk;

	if ((file = fopen(output_file, "wb")) == NULL) return ERROR;

	current_chunk = size > CHUNK_SIZE ? CHUNK_SIZE : size;
  while(size > 0 && (check = TCPRead(fd, buffer, current_chunk, TIMEOUT)) != ERROR){
    fwrite(buffer, sizeof(char), current_chunk, file);

		size -= current_chunk;
    current_chunk = size > CHUNK_SIZE ? CHUNK_SIZE : size;
  }
	fclose(file);

	return check == ERROR ? ERROR : TRUE;
}

void freePhoto(void * item){
	Photo * photo = (Photo *) item;
	char * image_file_name;

	// Apaga ficheiro da imagem
	image_file_name = getPhotoFileName(photo, NULL);
	unlink(image_file_name);
	free(image_file_name);

	free(photo->file_name);
	freeList(photo->keywords);
	free(item);
}

int comparePhotoWithPhotoId(void * a, void * id){
  if ( ((Photo *)a)->id == *(uint32_t *)id){
    return EQUAL;
  }
  return ((Photo *)a)->id > *(uint32_t *)id ? GREATER : SMALLER;
}

int compareChar(void * a, void * b){
  int res = strcmp((char *)a, (char *)b);

  if (res == 0) return EQUAL;
  return res > 0 ? GREATER : SMALLER;
}

// Registo na gateway
int registerAtGateway(int fd, struct sockaddr_in gateway_addr, in_port_t peer_port, struct in_addr peer_addr){
	int check;

	// Serializa informação a enviar
	check = serializePeerInfo(fd, gateway_addr, PEER_ID, peer_port, peer_addr);
	if (check == ERROR) return ERROR;

	// Aguarda resposta da gateway
	if ( deserializeInteger(fd, &check) != TRUE) return ERROR;

	return check;
}
int unregisterAtGateway(int fd, struct sockaddr_in gateway_addr, int peer_id){
	return serializePeerDeath(fd, gateway_addr, peer_id);
}


// Funções para leitura da socket
static int sendNewPhoto(int fd, Photo * photo){ //STATIC
	FILE * image_file;
	char buffer[CHUNK_SIZE];
	int size, nbytes;

	send(fd, &photo->id, sizeof(uint32_t), 0);

	size = strlen(photo->file_name);
	send(fd, &size, sizeof(int), 0);
	send(fd, photo->file_name, size*sizeof(char), 0);

	send(fd, &photo->size, sizeof(int), 0);

	image_file = fopen(getPhotoFileName(photo, buffer), "rb");
	size = photo->size;
	while ( (nbytes = fread(buffer, sizeof(char), CHUNK_SIZE, image_file) ) > 0) {
		if(send(fd, buffer, nbytes, MSG_NOSIGNAL) == -1) return ERROR;
	}
	fclose(image_file);

	return TRUE;
}
static Photo * receiveNewPhoto(int fd, uint32_t photo_id){
	Photo * photo;
	int size;
	char * buffer;

	photo = (Photo *) malloc(sizeof(Photo));
	photo->id = photo_id;
	photo->keywords = newList(compareChar, free);

	if ( TCPRead(fd, &size, sizeof(int), TIMEOUT) == ERROR) return NULL;
	photo->file_name = (char *) malloc((size+1)*sizeof(char));
	photo->file_name[size] = '\0';
	if ( TCPRead(fd, photo->file_name, size*sizeof(char), TIMEOUT) == ERROR) return NULL;

	if ( TCPRead(fd, &photo->size, sizeof(int), TIMEOUT) == ERROR) return NULL;

	buffer = getPhotoFileName(photo, NULL);
	if (readToFile(fd, photo->size, buffer) == ERROR) {free(buffer); return NULL;}

	free(buffer);

	return photo;
}

static int sendKeyword(int fd, uint32_t photo_id, char * keyword){ //STATIC
	int size;

	send(fd, &photo_id, sizeof(uint32_t), 0);

	size = strlen(keyword);
	send(fd, &size,sizeof(int), 0);
	send(fd, keyword, size*sizeof(char), 0);

	return TRUE;
}
static char * receiveKeyword(int fd){
	char * buffer;
	int size;

	TCPRead(fd, &size, sizeof(int), TIMEOUT);
	buffer = malloc((size+1)*sizeof(char));
	buffer[size] = '\0';

	TCPRead(fd, buffer, size*sizeof(char), TIMEOUT);

	return buffer;
}

static int sendPhotoId(int fd, uint32_t id){ //STATIC
	return send(fd, &id, sizeof(uint32_t), 0) == ERROR ? ERROR : TRUE;
}
static uint32_t receivePhotoId(int fd){
	uint32_t photo_id = ERROR;
	TCPRead(fd, &photo_id, sizeof(uint32_t), TIMEOUT);
	return photo_id;
}


int sendPhotoList(int fd, List * photos_list){
	Photo * photo;
	ListNode * current_node, * current_keyword;
	FILE * image_file;
	char * keyword, buffer[CHUNK_SIZE];
	int size, nbytes;

	size = getListSize(photos_list);
	send(fd, &size, sizeof(int), 0);

	current_node = getFirstListNode(photos_list);
	while(current_node != NULL){
		photo = (Photo *) getListNodeItem(current_node); // Obtem foto do current_node
		current_node = getNextListNode(current_node); // Avança na lista

		// Envia ID
		if ( send(fd, &photo->id, sizeof(uint32_t), 0) == ERROR) return ERROR;

		// Envia file_name
		size = strlen(photo->file_name);
		if ( send(fd, &size, sizeof(int), 0) == ERROR) return ERROR;
		if ( send(fd, photo->file_name, size*sizeof(char), 0) == ERROR) return ERROR;

		// Envia lista de keywords
		size = getListSize(photo->keywords);
		if ( send(fd, &size, sizeof(int), 0) == ERROR) return ERROR;

		current_keyword = getFirstListNode(photo->keywords);
		while(current_keyword != NULL){
			keyword = (char *) getListNodeItem(current_keyword);

			size = strlen(keyword);
			if ( send(fd, &size, sizeof(int), 0) == ERROR) return ERROR;
			if ( send(fd, keyword, size*sizeof(char), 0) == ERROR) return ERROR;

			current_keyword = getNextListNode(current_keyword);
		}

		getPhotoFileName(photo, buffer);
		image_file = fopen(buffer, "rb");

		fseek(image_file, 0, SEEK_END); // Permite a leitura do tamanho da imagem
		size = ftell(image_file);
		if ( send(fd, &size, sizeof(int), 0) == ERROR) return ERROR;

		fseek(image_file, 0, SEEK_SET); // Regressa ao inicio do ficheiro
		while ( (nbytes = fread(buffer, sizeof(char), CHUNK_SIZE, image_file) ) > 0) {
			if ( send(fd, buffer, nbytes, 0) == ERROR) return ERROR;
		}
		fclose(image_file);

	}

	return TRUE;
}
int receivePhotoList(int fd, List ** photo_list){
	FILE * image_file;
	Photo * photo;
	int i, j, size, msg_id, n_photos, n_keywords;
	char * keyword;
	char buffer[CHUNK_SIZE];
	int current_chunk;

	*photo_list = newList(comparePhotoWithPhotoId, freePhoto);

	// A socket não foi definida o que significa que não existem peers na gateway
	// Logo só é necessário alocar a lista de fotos
	if (fd == FALSE) return TRUE;

	msg_id = MSG_REPLICA_ALL;
	send(fd, &msg_id, sizeof(int), 0);

	if ( TCPRead(fd, &n_photos, sizeof(int), TIMEOUT) == ERROR) return ERROR;

	printf("Receiving %d images from existing peer...\n", n_photos);

	for (i = 0; i<n_photos; i++){
		photo = (Photo *) malloc(sizeof(Photo));

		// Recebe id da foto
		if ( TCPRead(fd, &photo->id, sizeof(uint32_t), TIMEOUT) == ERROR) return ERROR;

		// Recebe file_name
		if ( TCPRead(fd, &size, sizeof(int), TIMEOUT) == ERROR) return ERROR;
		photo->file_name = malloc((size+1)*sizeof(char));
		photo->file_name[size] = '\0';

		if ( TCPRead(fd, photo->file_name, size*sizeof(char), TIMEOUT) == ERROR) return ERROR;

		// Recebe lista de keywords
		photo->keywords = newList(compareChar, free);
		if ( TCPRead(fd, &n_keywords, sizeof(int), TIMEOUT) == ERROR) return ERROR;
		for(j = 0; j<n_keywords; j++){
			if ( TCPRead(fd, &size, sizeof(int), TIMEOUT) == ERROR) return ERROR;
			keyword = malloc((size+1)*sizeof(char));
			keyword[size] = '\0';

			if ( TCPRead(fd, keyword, size*sizeof(char), TIMEOUT) == ERROR) return ERROR;

			insertListItem(photo->keywords, keyword, keyword);
		}

		// Recebe imagem
		if ( TCPRead(fd, &photo->size, sizeof(int), TIMEOUT) == ERROR) return ERROR;
	  if ( (image_file = fopen(getPhotoFileName(photo, buffer), "wb")) == NULL) return ERROR;

		size = photo->size;
	  current_chunk = size > CHUNK_SIZE ? CHUNK_SIZE : size;
	  while(size > 0 && TCPRead(fd, buffer, current_chunk, TIMEOUT) != ERROR){
	    fwrite(buffer, sizeof(char), current_chunk, image_file);

			size -= current_chunk;
	    current_chunk = size > CHUNK_SIZE ? CHUNK_SIZE : size;
	  }
		fclose(image_file);

		insertListItem(*photo_list, photo, &photo->id);
	}

	return TRUE;
}

// Funções auxiliares
static int peer_identify_photo(int fd, List * photos_list, Photo ** photo){
  uint32_t id;
  int result;

  // Recebe id da foto
	if ((id = receivePhotoId(fd)) == ERROR) return ERROR;

  // Procura a foto na lista (e devolve-a por referencia)
  *photo = (Photo *) findListItem(photos_list, &id);
  result = (*photo == NULL) ? FALSE : (*photo)->size;

  // Envia resultado
  if (send(fd, &result, sizeof(int), 0) == ERROR) return ERROR;

  return result;
}
static uint32_t getNewPhotoID(){
	uint32_t new_id;
	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

	// Abre um socket UDP para comunicar com a gateway
	if (serializeInteger(udp_socket, GATEWAY_SOCKADDR, MSG_GATEWAY_NEW_PHOTO_ID) == ERROR ||
			deserializeUint32(udp_socket, &new_id) == ERROR) return ERROR;

	printf("Novo ID da foto: %d\n", new_id);

	// Fecha a socket
	close(udp_socket);

	return new_id;
}

static int send_replica(int (*routine) (int, Photo *, char *), Photo * photo, char * keyword){
	int fd_gateway, fd_peer;
	int n_peers, i;

	struct sockaddr_in peer_sockaddr;


	if ((fd_gateway = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) return ERROR;

	// Liga-se ao porto TCP da gateway com uma nova socket
	if (connect(fd_gateway, (struct sockaddr *) &GATEWAY_SOCKADDR, sizeof(GATEWAY_SOCKADDR)) == ERROR) return ERROR;

	// Envia ID do peer
	send(fd_gateway, &PEER_ID, sizeof(int), 0);

	// Recebe número de peers
	TCPRead(fd_gateway, &n_peers, sizeof(int), TIMEOUT);

	for(i = 0; i<n_peers; i++){
		memset(&peer_sockaddr, 0, sizeof(struct sockaddr_in));
		
		TCPRead(fd_gateway, &peer_sockaddr.sin_port, sizeof(in_port_t), TIMEOUT);
		TCPRead(fd_gateway, &peer_sockaddr.sin_addr, sizeof(struct in_addr), TIMEOUT);
		peer_sockaddr.sin_family = AF_INET;

		// Abre nova socket TCP
		fd_peer = socket(AF_INET, SOCK_STREAM, 0);

		// Conecta-se como cliente ao peer recebido
		connect(fd_peer, (struct sockaddr *) &peer_sockaddr, sizeof(struct sockaddr));

		// Executa a rotina de envio especificada
		routine(fd_peer, photo, keyword);

		// Fecha a socket
		close(fd_peer);
	}

	return TRUE;
}
static int send_replica_new_photo(int fd, Photo * photo, char * keyword){
	int msg_id = MSG_REPLICA_NEW_PHOTO;

	send(fd, &msg_id, sizeof(int), 0);
	sendNewPhoto(fd, (Photo *) photo);

	return TRUE;
}
static int send_replica_add_keyword(int fd, Photo * photo, char * keyword){
	int msg_id = MSG_REPLICA_ADD_KEYWORD;

	send(fd, &msg_id, sizeof(int), 0);
	sendKeyword(fd, photo->id, keyword);

	return TRUE;
}
static int send_replica_delete_photo(int fd, Photo * photo, char * keyword){
	int msg_id = MSG_REPLICA_DELETE_PHOTO;

	send(fd, &msg_id, sizeof(int), 0);
	sendPhotoId(fd, photo->id);

	return TRUE;
}

// Funções que permite interagir com o cliente
int handle_new_photo(int fd, List * photos_list) {
	//int peer_add_photo_client(int fd, List * photos_list){
	Photo * new_photo;
	uint32_t photo_id;

	if ((photo_id = getNewPhotoID()) == ERROR) return ERROR;
	if ((new_photo = receiveNewPhoto(fd, photo_id)) == NULL) return ERROR;

	// Insere foto na lista
	insertListItem(photos_list, new_photo, &new_photo->id);

	// Envia id da imagem
  if ( (send(fd, &new_photo->id, sizeof(uint32_t), 0)) == ERROR) return ERROR;

	send_replica(send_replica_new_photo, new_photo, NULL);
  return TRUE;
}
int handle_add_keyword(int fd, List * photos_list){
  Photo * photo;
	char * keyword;
  int check;

	// Obtem foto (e responde ao cliente)
  check = peer_identify_photo(fd, photos_list, &photo);
  if (check == ERROR) return ERROR;
	if (check == FALSE) return FALSE;


	if ((keyword = receiveKeyword(fd)) == NULL) return ERROR;
  insertListItem(photo->keywords, keyword, keyword); // A string é o elemento a armazenar e simultaneamente o id

	send_replica(send_replica_add_keyword, photo, keyword);
  return TRUE;
}
int handle_search_photo(int fd, List * photos_list){
	Photo * photo;
	List * id_list;
	ListNode * current_node;
	char * buffer;
	int size, check;
	uint32_t * id;

	// Recebe tamanho de keyword
  check = TCPRead(fd, &size, sizeof(int), TIMEOUT);
  if (check == ERROR) return ERROR;

	buffer = (char *) malloc((size + 1)*sizeof(char));
	buffer[size] = '\0';

	// Recebe keyword
  check = TCPRead(fd, buffer, size*sizeof(char), TIMEOUT);
  if (check == ERROR) return ERROR;

	// Iteração em todas as fotos da lista
	id_list = newList(NULL, NULL);

	current_node = getFirstListNode(photos_list);
	while(current_node != NULL){
		photo = (Photo *) getListNodeItem(current_node);
		current_node = getNextListNode(current_node);

		if (findListItem(photo->keywords, buffer) != NULL){
			insertListItem(id_list, &photo->id, NULL);
		}
	}

	free(buffer);

	// Envia nº de resultados
	size = getListSize(id_list);
  check = send(fd, &size, sizeof(int), 0);
  if (check == -1) return ERROR;

	// Envia os ids das fotos encontradas
	current_node = getFirstListNode(id_list);
	while(current_node != NULL){
		id = getListNodeItem(current_node);
		current_node = getNextListNode(current_node);

		if ( send(fd, id, sizeof(uint32_t), 0) == ERROR) return ERROR;
	}

	freeList(id_list);

	return TRUE;
}
int handle_delete_photo(int fd, List * photos_list){
	Photo * photo;
	int check;

	check = peer_identify_photo(fd, photos_list, &photo);
  if (check == ERROR || check == FALSE) return check;

	removeListItem(photos_list, &photo->id);
	freePhoto(photo);

	send_replica(send_replica_delete_photo, photo, NULL);
	return TRUE;
}
int handle_get_photo_name(int fd, List * photos_list){
	Photo * photo;
	int check, size;

	check = peer_identify_photo(fd, photos_list, &photo);
  if (check == ERROR) return ERROR;
	if (check == FALSE) return FALSE;

	// Envia tamanho da string file_name
	size = strlen(photo->file_name);

  check = send(fd, &size, sizeof(int), 0);
  if (check == -1) return ERROR;

	// Envia string file_name
	check = send(fd, photo->file_name, size*sizeof(char), 0);
  if (check == -1) return ERROR;

	return TRUE;
}
int handle_get_photo(int fd, List * photos_list){
	FILE * image_file;
	char * image_file_name;
	Photo * photo;
	char buffer[CHUNK_SIZE];
	int size, check, nbytes;

	size = peer_identify_photo(fd, photos_list, &photo);
  if (size == ERROR) return ERROR;
	if (size == FALSE) return FALSE;

	// Abre ficheiro em modo leitura
	image_file_name = getPhotoFileName(photo, NULL);
	image_file = fopen(image_file_name, "rb");
	free(image_file_name);
	if (image_file == NULL) return ERROR;

	// Confirma se a imagem não sofreu alterações no servidor
	fseek(image_file, 0, SEEK_END);
	if (size != ftell(image_file)) return ERROR; // A imagem foi alterada no servidor -> erro

	// Envia ficheiro da imagem
	fseek(image_file, 0, SEEK_SET);
	while ( (nbytes = fread(buffer, sizeof(char), CHUNK_SIZE, image_file) ) > 0) {
		check = send(fd, buffer, nbytes, 0);
		if (check == -1) return 0;
	}

	fclose(image_file);

	return TRUE;
}

int handle_replica_new_photo(int fd, List * photos_list){
	Photo * photo;
	uint32_t photo_id;

	// Recebe id da foto
	if ( TCPRead(fd, &photo_id, sizeof(uint32_t), TIMEOUT) == ERROR) return ERROR;

	// Recebe restante informação da foto
	if ((photo = receiveNewPhoto(fd, photo_id)) == NULL) return ERROR;

	insertListItem(photos_list, photo, &photo->id);
	return TRUE;
}
int handle_replica_add_keyword(int fd, List * photos_list){
	Photo * photo;
	uint32_t photo_id;
	char * keyword;

	// Recebe id da foto
	if ((photo_id = receivePhotoId(fd)) == ERROR) return ERROR;

	// Recebe keyword
	if ((keyword = receiveKeyword(fd)) == NULL) return ERROR;
	if ((photo = findListItem(photos_list, &photo_id)) == NULL) return ERROR;

	insertListItem(photo->keywords, keyword, keyword);
	return TRUE;
}
int handle_replica_delete_photo(int fd, List * photos_list){
	uint32_t photo_id;

	if ((photo_id = receivePhotoId(fd)) == ERROR) return ERROR;

	removeListItem(photos_list, &photo_id);
	return TRUE;
}
