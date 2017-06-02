CC=gcc
CFLAGS=-Wall -g -pthread
DEPS = global.h
PROGRAMS = client peer gateway

default: client peer gateway

client: client.o api.o serializer.o
	$(CC) -o client client.o api.o serializer.o $(CFLAGS)

peer: peer.o serializer.o peer_api.o api.o Estruturas/headed_list.o
	$(CC) -o peer peer.o serializer.o peer_api.o api.o Estruturas/headed_list.o  $(CFLAGS)

gateway: gateway.o Estruturas/headed_list.o serializer.o
	$(CC) -o gateway gateway.o Estruturas/headed_list.o serializer.o $(CFLAGS)


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


clean:
	rm -f *.o *.out Estruturas/*.o $(PROGRAMS)


run_peer: peer
	sudo ./peer 127.0.0.1 1000 2000 127.0.0.1 4000
 #sudo ./peer 'addr_gateway' 'client_gateway_port' 'peer_gateway_port' 'addr_peer' 'port_peer'

run_client: client
	./client 127.0.0.1 1000
 #./client 'addr_gateway' 'port_gateway'

run_gateway: gateway
	sudo ./gateway 127.0.0.1 1000 2000
 #sudo ./gateway 'addr' 'clients_port' 'peers_port'

debug_peer: peer
	sudo ddd ./peer

debug_client: client
	ddd ./client

debug_gateway: gateway
	sudo ddd ./gateway

# Descobrir qual o meu ip -> $ ip addr
# nohup
