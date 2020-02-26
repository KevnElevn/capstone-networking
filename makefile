server : server.o Protocol.o
	g++ -o server server.o Protocol.o

client : client.o Protocol.o
	g++ -o client client.o Protocol.o

server.o: Protocol.h

client.o: Protocol.h

Protocol.o: Protocol.h
