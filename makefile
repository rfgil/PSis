default: Estruturas/list.c
	gcc -Wall -g -pthread api.c Estruturas/headed_list.c -o api

gateway:
	gcc -Wall -g -pthread -std=c99 -D_XOPEN_SOURCE gateway.c Estruturas/list.c -o gateway
