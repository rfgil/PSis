#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

#define TRUE 1
#define FALSE 0

//#define GATEWAY_ADDRESS "aaa"
#define GATEWAY_PORT_CLIENTS 3000
#define GATEWAY_PORT_PEERS 4000

#define MAX_MSG_LEN 100

typedef struct message_gw{
	int type;
	char address[20];
	int port;
} GatewayMsg;


void checkError(int var, char * description){
	if(var == -1) {
		perror(description);
		exit(-1);
	}
}

char * serialize(GatewayMsg * msg){
	char * buffer;
	buffer = malloc(sizeof(GatewayMsg));
	memcpy(buffer, msg, sizeof(GatewayMsg));
	return buffer;
}

GatewayMsg * deserialize(char * buffer){
	GatewayMsg * msg;
	msg = malloc(sizeof(GatewayMsg));
	memcpy(msg, buffer, sizeof(GatewayMsg));
	return msg;
}

#endif

