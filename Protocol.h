#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <string>

std::string bufferToString(char* buff, int buffSize);
int addSeqAckNumber(int seq, int num);
std::string createSynPacket(int seq, int bufferSize, int maxMessageSize);
std::string createSynAckPacket(int seq, int ack, int bufferSize, int maxMessageSize, std::string& buff);
std::string createAckPacket(int seq, int ack);
std::string createRstPacket(int seq, int ack);

#endif
