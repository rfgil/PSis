CC=gcc
CFLAGS=-Wall -g -pthread
DEPS = global.h
PROGRAMS = client peer

client: client.o api.o
	$(CC) -o client client.o api.o $(CFLAGS)

peer: peer.o peer_api.o Estruturas/headed_list.o
	$(CC) -o peer peer.o peer_api.o Estruturas/headed_list.o $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o *.out Estruturas/*.o $(PROGRAMS)

run_peer: peer
	sudo ./peer 127.0.0.1 1000

run_client: client
	./client 127.0.0.1 1000

debug_peer: peer
	sudo ddd ./peer

debug_client: client
	ddd ./client


# 192.168.0.255
# Descobrir qual o meu ip -> $ ip addr

# nohup
