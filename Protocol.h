#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <string>

std::string bufferToString(char* buff, int buffSize);
int addSeqAckNumber(int seq, int num);
void printPacket(std::string& buffer);
std::string createSynPacket(int seq, int bufferSize, int maxMessageSize);
std::string createSynAckPacket(int seq, int ack, int bufferSize, int maxMessageSize);
std::string createCtrlPacket(char type, int seq, int ack);
std::string createDatPacket(int seq, int ack, int messageLength, std::string& data);
bool receiveSynPacket(int& seq, int& ack, int& maxBuffer, int& maxMessage, std::string& buffer);
bool receiveSynAckPacket(int& seq, int& ack, int& maxBuffer, int& maxMessage, std::string& buffer);
bool receiveAckPacket(int& seq, int& ack, std::string& buffer);
int receiveDatPacket(std::string& buffer, std::string& fullMessage);
void sendRstPacket(int socket, std::string sendStr, int seq, int ack);
const int PACKET_TYPE_LENGTH = 1;
const int BUFFER_SIZE_LENGTH = 2;
const int MESSAGE_SIZE_LENGTH = 2;
const int SEQ_ACK_LENGTH = 6;
const int MESSAGE_END_LENGTH = 9;
const int DATA_HEADER_LENGTH = 22;

const char DAT = '0';
const char SYN = '1';
const char SYNACK = '2';
const char ACK = '3';
const char FIN = '7';
const char FINACK = '8';
const char RST = '9';

#endif
