default: Estruturas/list.c
	gcc -Wall -g -pthread -std=c99 -D_XOPEN_SOURCE gateway.c Estruturas/list.c -o gateway
