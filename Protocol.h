#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <string>
#include <unordered_map>
#include "Packet.h"

int addSeqAckNumber(int seq, int num);
void sendPacket(int socket, Packet& packet);
char recvPacket(int socket, char* buffer, int bufferSize, Packet& packet);

const int PACKET_TYPE_LENGTH = 1;
const int BUFFER_SIZE_LENGTH = 2;
const int MESSAGE_SIZE_LENGTH = 2;
const int SEQ_ACK_LENGTH = 6;
const int CHUNK_ID_LENGTH = 9;
const int FILE_SIZE_LENGTH = 9;
const int MESSAGE_LENGTH = 9;
const int DATA_HEADER_LENGTH = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+MESSAGE_LENGTH;
const int RST_ERROR_LENGTH = 2;

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

const std::unordered_map<int, std::string> RSTERRORS({
  {0, "Missing error code"},
  {1, "Expected SYN packet"},
  {2, "Expected SAK packet"},
  {3, "Expected ACK packet"},
  {4, "Filename too long to send with current buffer size. Try again with a bigger buffer."},
  {5, "File not found"},
  {6, "File is too large for current MaxMessage value"},
  {7, "Expected RAK packet"},
  {8, "Different filename acknowledged"},
  {9, "Unable to open writeFile"},
  {10, "Expected DON packet"},
  {11, "Expected FAK packet"},
  {12, "Out of sequence packet"},
  {97, "No data receieved"},
  {98, "Wait for response timed out"},
  {99, "Bad acknowledge number"}
});

#endif
