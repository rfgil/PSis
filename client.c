#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <unistd.h>
#include <arpa/inet.h>

void checkError(int var, char * description){
	if(var == -1) {
		perror(description);
		exit(-1);
	}
}

int main(){
	
	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	checkError(sock_fd, "socket");
	
	int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	checkError(err, "bind");
	
	

	
	
	return 0;
}
