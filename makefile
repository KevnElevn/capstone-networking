server : server.o Protocol.o Packet.o
	g++ -o server server.o Protocol.o Packet.o

client : client.o Protocol.o Packet.o
	g++ -o client client.o Protocol.o Packet.o

server.o: Protocol.h

client.o: Protocol.h

Protocol.o: Protocol.h

Packet.o: Packet.h
