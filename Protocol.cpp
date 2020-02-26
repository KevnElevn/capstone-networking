#include "Protocol.h"
#include <string>

std::string bufferToString(char* buff, int buffSize) {
  std::string contents;
  for(int i=0; i<buffSize; i++) {
    contents += buff[i];
  }
  return contents;
}

int addSeqAckNumber(int target, int num) {
  target += num;
  if(target > 999999)
    target -= 1000000;
  return target;
}

std::string createSynPacket(int seq, int maxBufferSize, int maxMessageSize) {
  std::string sequence = std::to_string(seq);
  std::string bufferSizeStr = std::to_string(maxBufferSize);
  std::string messageSizeStr = std::to_string(maxMessageSize);
  sequence.insert(sequence.begin(), 6-sequence.size(), '0');
  bufferSizeStr.insert(bufferSizeStr.begin(), 2-bufferSizeStr.size(), '0');
  messageSizeStr.insert(messageSizeStr.begin(), 2-messageSizeStr.size(), '0');
  return "SYN" + sequence + bufferSizeStr + messageSizeStr;
}

std::string createSynAckPacket(int seq, int ack, int bufferSize, int maxMessageSize, std::string& buff) {
  std::string sequence = std::to_string(seq);
  std::string acknowledge = std::to_string(ack);
  std::string bufferSizeStr = std::to_string(bufferSize);
  std::string messageSizeStr = std::to_string(maxMessageSize);
  acknowledge = std::to_string(ack);
  sequence.insert(sequence.begin(), 6-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), 6-acknowledge.size(), '0');
  bufferSizeStr.insert(bufferSizeStr.begin(), 2-bufferSizeStr.size(), '0');
  messageSizeStr.insert(messageSizeStr.begin(), 2-messageSizeStr.size(), '0');
  return "SAK" + sequence + acknowledge + bufferSizeStr + messageSizeStr;
}

std::string createAckPacket(int seq, int ack) {
  std::string sequence = std::to_string(seq);
  std::string acknowledge = std::to_string(ack);
  sequence.insert(sequence.begin(), 6-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), 6-acknowledge.size(), '0');
  return "ACK" + sequence + acknowledge;
}

std::string createRstPacket(int seq, int ack) {
  std::string sequence = std::to_string(seq);
  std::string acknowledge = std::to_string(ack);
  sequence.insert(sequence.begin(), 6-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), 6-acknowledge.size(), '0');
  return "RST" + sequence + acknowledge;
}
