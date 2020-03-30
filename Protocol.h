#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <string>
#include "Packet.h"

int addSeqAckNumber(int seq, int num);
bool checkAck(int& seq, int& ack, Packet& packet);
void sendPacket(int socket, Packet& packet);
int recvPacket(int socket, char* buffer, int bufferSize, Packet& packet);

const int PACKET_TYPE_LENGTH = 1;
const int BUFFER_SIZE_LENGTH = 2;
const int MESSAGE_SIZE_LENGTH = 2;
const int SEQ_ACK_LENGTH = 6;
const int CHUNK_ID_LENGTH = 9;
const int FILE_SIZE_LENGTH = 9;
const int MESSAGE_LENGTH = 9;
const int DATA_HEADER_LENGTH = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+MESSAGE_LENGTH;

const char DAT = '0';
const char SYN = '1';
const char SAK = '2';
const char ACK = '3';
const char REQ = '4';
const char RAK = '5';
const char DON = '6';
const char FIN = '7';
const char FAK = '8';
const char RST = '9';

#endif
