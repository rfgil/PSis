#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

#define TRUE 1
#define FALSE 0

#define GATEWAY_ADDRESS "aaa"
#define GATEWAY_PORT 3000

#define MAX_MSG_LEN 100

typedef struct message_gw{
	int type;
	char address[20];
	int port;
} GatewayMsg;

#endif

