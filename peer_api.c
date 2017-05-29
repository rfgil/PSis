#include <sys/types.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "peer_api.h"
#include "global.h"


void freePhoto(void * item){
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
  check = myRead(fd, &id, sizeof(uint32_t), TIMEOUT);
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
  unsigned char buffer[CHUNK_SIZE];
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
  check = myRead(fd, &size, sizeof(int), TIMEOUT);
	if (check == ERROR) return ERROR;

  // Aloca espaço para armazenar file_name
  new_photo->file_name = (char *) malloc((size+1)*sizeof(char));
  new_photo->file_name[size] = '\0';

  // Recebe file_name
  check = myRead(fd, new_photo->file_name, size*sizeof(char), TIMEOUT);
	if (check == ERROR) return ERROR;

  // Recebe tamanho da imagem
  check = myRead(fd, &size, sizeof(int), TIMEOUT);
  if (check == ERROR || size <= 0) return ERROR;

	new_photo->size = size;

  // Escreve imagem recebida para ficheiro
  image_file = fopen(strcat(PEER_FOLDER_PREFIX, new_photo->file_name), "wb");
  current_chunk = size > CHUNK_SIZE ? CHUNK_SIZE : size;
  while(size > 0 && (check = myRead(fd, buffer, current_chunk, TIMEOUT)) != ERROR){
    fwrite(buffer, sizeof(char), current_chunk, image_file);
    current_chunk = size > CHUNK_SIZE ? CHUNK_SIZE : size;
  }
  if (check == ERROR) return ERROR;

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
  check = myRead(fd, &size, sizeof(int), TIMEOUT);
  if (check == ERROR) return ERROR;

  buffer = (char *) malloc((size + 1)*sizeof(char));
  buffer[size]  = '\0';

  // Recebe file_name
  check = myRead(fd, buffer, size*sizeof(char), TIMEOUT);
  if (check == ERROR) return ERROR;

  insertListItem(photo->keywords, buffer, buffer); // A string é o elemento a armazenar e simultaneamente o id

  return TRUE;
}

int peer_search_photo(int fd, List * photos_list){
	Photo * photo;
	List * id_list;
	char * buffer;
	int size, check;
	uint32_t * id;

	// Recebe tamanho de keyword
  check = myRead(fd, &size, sizeof(int), TIMEOUT);
  if (check == ERROR) return ERROR;

	buffer = (char *) malloc((size + 1)*sizeof(char));
	buffer[size] = '\0';

	// Recebe keyword
  check = myRead(fd, buffer, size*sizeof(char), TIMEOUT);
  if (check == ERROR) return ERROR;

	// Iteração em todas as fotos da lista
	id_list = newList(NULL, NULL);
	startListIteration(photos_list);
	while((photo = (Photo *) getListNextItem(photos_list)) != NULL){
		// Procura foto com id igual à keyword recebida
		if (findListItem(photo->keywords, buffer) != NULL){
			insertListItem(id_list, &photo->id, NULL);
		}
	}

	// Envia nº de resultados
	size = getListSize(id_list);
  check = send(fd, &size, sizeof(int), 0);
  if (check == -1) return ERROR;

	// Envia os ids das fotos encontradas
	startListIteration(id_list);
	while((id = (uint32_t *) getListNextItem(id_list)) != NULL){
		check = send(fd, id, sizeof(uint32_t), 0);
	  if (check == -1) return ERROR;
	}

	return TRUE;
}

int peer_delete_photo(int fd, List * photos_list){
	Photo * photo;
	int check;

	check = peer_identify_photo(fd, photos_list, &photo);
  if (check == ERROR) return ERROR;
	if (check == FALSE) return FALSE;

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
	Photo * photo;
	char buffer[CHUNK_SIZE];
	int size, check, nbytes;

	size = peer_identify_photo(fd, photos_list, &photo);
  if (size == ERROR) return ERROR;
	if (size == FALSE) return FALSE;

	// Abre ficheiro em modo leitura
	image_file = fopen(strcat(PEER_FOLDER_PREFIX, photo->file_name), "rb");
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

	return TRUE;
}
