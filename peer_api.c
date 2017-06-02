#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

int getNewPhotoID(List * photos_list){
  // Para já o novo id será o tamanho da lista
  return getListSize(photos_list);
}


static int peer_identify_photo(int fd, List * photos_list, Photo ** photo){
  uint32_t id;
  int check, result;

  // Informação enviada:
  // int - identificador da mensagem (já recebido)
  // int - id da imagem
  // Aguarda confirmação de que a imagem existe (int)

  // Recebe id da foto
  check = TCPRead(fd, &id, sizeof(uint32_t), TIMEOUT);
	if (check == ERROR) return ERROR;

  // Procura a foto na lista
  *photo = (Photo *) findListItem(photos_list, &id);

  result = (*photo == NULL) ? FALSE : (*photo)->size;

  // Envia resultado
  check = send(fd, &result, sizeof(int), 0);
  if (check == -1) return ERROR;

  return result;
}

int peer_add_photo_client(int fd, List * photos_list){
  Photo * new_photo;
  FILE * image_file;
	char * image_file_name;
  char buffer[CHUNK_SIZE];
  int check, size, current_chunk;

  // Informação enviada:
  // int - identificador da mensagem (já recebido)
  // int - tamanho da string file_name
  // char - file_name
  // int - tamanho da imagem
  // char - imagem

  new_photo = (Photo *) malloc(sizeof(Photo));
  new_photo->id = getNewPhotoID(photos_list);
  new_photo->keywords = newList(compareChar, free); // lista de chars

  // Recebe tamanho de file_name
  check = TCPRead(fd, &size, sizeof(int), TIMEOUT);
	if (check == ERROR) return ERROR;

  // Aloca espaço para armazenar file_name
  new_photo->file_name = (char *) malloc((size+1)*sizeof(char));
  new_photo->file_name[size] = '\0';

  // Recebe file_name
  check = TCPRead(fd, new_photo->file_name, size*sizeof(char), TIMEOUT);
	if (check == ERROR) return ERROR;

  // Recebe tamanho da imagem
  check = TCPRead(fd, &new_photo->size, sizeof(int), TIMEOUT);
  if (check == ERROR || size <= 0) return ERROR;

  // Escreve imagem recebida para ficheiro
	image_file_name = getPhotoFileName(new_photo, NULL);
  image_file = fopen(image_file_name, "wb");
	free(image_file_name);
	if (image_file == NULL) return ERROR;

	size = new_photo->size;
  current_chunk = size > CHUNK_SIZE ? CHUNK_SIZE : size;
  while(size > 0 && (check = TCPRead(fd, buffer, current_chunk, TIMEOUT)) != ERROR){
    fwrite(buffer, sizeof(char), current_chunk, image_file);

		size -= current_chunk;
    current_chunk = size > CHUNK_SIZE ? CHUNK_SIZE : size;
  }
	fclose(image_file);
  if (check == ERROR) return ERROR;


	// Insere foto na lista
	insertListItem(photos_list, new_photo, &new_photo->id);

	// Envia id da imagem
  check = send(fd, &new_photo->id, sizeof(uint32_t), 0);
  if (check == -1) return ERROR;

  return TRUE;
}

int peer_add_keyword(int fd, List * photos_list){
  Photo * photo;
  char * buffer;
  int check, size;

  check = peer_identify_photo(fd, photos_list, &photo);
  if (check == ERROR) return ERROR;
	if (check == FALSE) return FALSE;

  // Recebe tamanho de file_name
  check = TCPRead(fd, &size, sizeof(int), TIMEOUT);
  if (check == ERROR) return ERROR;

  buffer = (char *) malloc((size + 1)*sizeof(char));
  buffer[size]  = '\0';

  // Recebe file_name
  check = TCPRead(fd, buffer, size*sizeof(char), TIMEOUT);
  if (check == ERROR) return ERROR;

  insertListItem(photo->keywords, buffer, buffer); // A string é o elemento a armazenar e simultaneamente o id

  return TRUE;
}

int peer_search_photo(int fd, List * photos_list){
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

int peer_delete_photo(int fd, List * photos_list){
	Photo * photo;
	int check;

	check = peer_identify_photo(fd, photos_list, &photo);
  if (check == ERROR || check == FALSE) return check;

	removeListItem(photos_list, &photo->id);
	freePhoto(photo);

	return TRUE;
}

int peer_get_photo_name(int fd, List * photos_list){
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

int peer_get_photo(int fd, List * photos_list){
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
