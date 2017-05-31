#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "global.h"
#include "api.h"

static int getUserInput(char * message, char * buffer){
  printf("%s", message);
  if (fgets(buffer, CHUNK_SIZE, stdin) == NULL) return ERROR;
  buffer[strlen(buffer) - 1] = '\0';
  return TRUE;
}

int client_add_photo(int fd, char * buffer){
  getUserInput("Photo file: ", buffer);
  return gallery_add_photo(fd, buffer);
}

int client_add_keyword(int fd, char * buffer){
  uint32_t id;

  do {
    getUserInput("ID da foto: ", buffer);
  } while(sscanf(buffer, "%d ", &id) != 1);

  getUserInput("Keyword: ", buffer);

  return gallery_add_keyword(fd, id, buffer);
}

int client_search_photo(int fd, char * buffer){
  uint32_t * id_photos;
  int count, i;

  getUserInput("Keyword: ", buffer);

  count = gallery_search_photo(fd, buffer, &id_photos);
  if (count == ERROR) return ERROR;

  printf("Found ids: ");
  for(i = 0; i<count; i++){
    printf("%d ", id_photos[i]);
  }
  printf("\n");

  free(id_photos);

  return TRUE;
}

int client_delete_photo(int fd, char * buffer){
  uint32_t id;

  do {
    getUserInput("ID da foto: ", buffer);
  } while(sscanf(buffer, "%d ", &id) != 1);

  return gallery_delete_photo(fd, id);
}

int client_get_photo_name(int fd, char * buffer){
  uint32_t id;
  char * photo_name;
  int check;

  do {
    getUserInput("ID da foto: ", buffer);
  } while(sscanf(buffer, "%d ", &id) != 1);

  check = gallery_get_photo_name(fd, id, &photo_name);
  if (check == ERROR || check == FALSE) return check;

  printf("Photo name: %s\n", photo_name);
  free(photo_name);

  return TRUE;
}

int client_get_photo(int fd, char * buffer){
  uint32_t id;

  do {
    getUserInput("ID da foto: ", buffer);
  } while(sscanf(buffer, "%d ", &id) != 1);

  getUserInput("Guardar no ficheiro: ", buffer);

  return gallery_get_photo(fd, id, buffer);
}


int main(int argc, char *argv[]){
  char buffer[CHUNK_SIZE];
  int command, check;
  int fd = ERROR; // Não inicializada por defeito

  assert(argc > 2);

  fd = gallery_connect(argv[1], htons(atoi(argv[2])));
  if (fd == ERROR) { perror(NULL); return 1; }

  do {
    printf("Choose command:\n"
           "1. Add Photo - arg1: file_name\n"
           "2. Add Keyword\n"
           "3. Search Photo\n"
           "4. Delete Photo\n"
           "5. Get Photo Name\n"
           "6. Get Photo\n"
           "0. Quit\n");

    if ( fgets(buffer, CHUNK_SIZE, stdin) == NULL) continue;
    if ( sscanf(buffer, "%d ", &command) != 1 ) command = -1;

    switch (command) {
      case 0:
        break;

      case 1: // Add Photo
        check = client_add_photo(fd, buffer);
        break;

      case 2: // Add Keyword
        check = client_add_keyword(fd, buffer);
        break;

      case 3: // Search Photo
        check = client_search_photo(fd, buffer);
        break;

      case 4: // Delete Photo
        check = client_delete_photo(fd, buffer);
        break;

      case 5: // Get Photo Name
        check = client_get_photo_name(fd, buffer);
        break;

      case 6: // Get Photo
        check = client_get_photo(fd, buffer);
        break;

      default:
        printf("Invalid command!\n");
        continue;
    }

    if (check == ERROR){
      command = 0; // Termina aplicação
      
      printf("An error occurred.\n");
      perror(NULL);
    } else {
      printf("Done! (%d)\n", check);
    }

  } while(command != 0); // Diferente de QUIT

  close(fd);

  return 0;
}
