#include "Protocol.h"
#include <sys/socket.h>
#include <string>
#include <iostream>

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

void printPacket(std::string& buffer) {
  switch(buffer[0]) {
    case DAT:
    std::cout << "DAT\n";
    std::cout << "Sequence number: " << buffer.substr(PACKET_TYPE_LENGTH, SEQ_ACK_LENGTH) << std::endl;
    std::cout << "Acknowledge number: " << buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH, SEQ_ACK_LENGTH) << std::endl;
    std::cout << "Message length: " << buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), MESSAGE_END_LENGTH) << std::endl;
    std::cout << "Message: " << buffer.substr(DATA_HEADER_LENGTH, atoi(buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), MESSAGE_END_LENGTH).data())) << std::endl;
    break;

    case SYN:
    std::cout << "SYN\n";
    std::cout << "Squence number: " << buffer.substr(PACKET_TYPE_LENGTH, SEQ_ACK_LENGTH) << std::endl;
    std::cout << "Max buffer size: " << buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH, BUFFER_SIZE_LENGTH) << std::endl;
    std::cout << "Max message size: " << buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH ,MESSAGE_SIZE_LENGTH) << std::endl;
    break;

    case SYNACK:
    std::cout << "SYNACK\n";
    std::cout << "Sequence number: " << buffer.substr(PACKET_TYPE_LENGTH, SEQ_ACK_LENGTH) << std::endl;
    std::cout << "Acknowledge number: " << buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH, SEQ_ACK_LENGTH) << std::endl;
    std::cout << "Max buffer size: " << buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), BUFFER_SIZE_LENGTH) << std::endl;
    std::cout << "Max message size: " << buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH, MESSAGE_SIZE_LENGTH) << std::endl;
    break;

    case ACK:
    std::cout << "ACK\n";
    std::cout << "Sequence number: " << buffer.substr(PACKET_TYPE_LENGTH, SEQ_ACK_LENGTH) << std::endl;
    std::cout << "Acknowledge number: " << buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH, SEQ_ACK_LENGTH) << std::endl;
    break;

    case RST:
    std::cout << "RST\n";
    std::cout << "Sequence number: " << buffer.substr(PACKET_TYPE_LENGTH, SEQ_ACK_LENGTH) << std::endl;
    std::cout << "Acknowledge number: " << buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH, SEQ_ACK_LENGTH) << std::endl;
    break;
  }
}

std::string createSynPacket(int seq, int maxBufferSize, int maxMessageSize) {
  std::string sequence = std::to_string(seq);
  std::string bufferSizeStr = std::to_string(maxBufferSize);
  std::string messageSizeStr = std::to_string(maxMessageSize);
  sequence.insert(sequence.begin(), SEQ_ACK_LENGTH-sequence.size(), '0');
  bufferSizeStr.insert(bufferSizeStr.begin(), BUFFER_SIZE_LENGTH-bufferSizeStr.size(), '0');
  messageSizeStr.insert(messageSizeStr.begin(), MESSAGE_SIZE_LENGTH-messageSizeStr.size(), '0');
  return SYN + sequence + bufferSizeStr + messageSizeStr;
}

std::string createSynAckPacket(int seq, int ack, int bufferSize, int maxMessageSize) {
  std::string sequence = std::to_string(seq);
  std::string acknowledge = std::to_string(ack);
  std::string bufferSizeStr = std::to_string(bufferSize);
  std::string messageSizeStr = std::to_string(maxMessageSize);
  acknowledge = std::to_string(ack);
  sequence.insert(sequence.begin(), SEQ_ACK_LENGTH-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), SEQ_ACK_LENGTH-acknowledge.size(), '0');
  bufferSizeStr.insert(bufferSizeStr.begin(), BUFFER_SIZE_LENGTH-bufferSizeStr.size(), '0');
  messageSizeStr.insert(messageSizeStr.begin(), MESSAGE_SIZE_LENGTH-messageSizeStr.size(), '0');
  return SYNACK + sequence + acknowledge + bufferSizeStr + messageSizeStr;
}

std::string createCtrlPacket(char type, int seq, int ack) {
  std::string sequence = std::to_string(seq);
  std::string acknowledge = std::to_string(ack);
  sequence.insert(sequence.begin(), SEQ_ACK_LENGTH-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), SEQ_ACK_LENGTH-acknowledge.size(), '0');
  return type + sequence + acknowledge;
}

std::string createDatPacket(int seq, int ack, int messageLength, std::string& data) {
  std::string sequence = std::to_string(seq);
  std::string acknowledge = std::to_string(ack);
  std::string messageEnd = std::to_string(messageLength);
  sequence.insert(sequence.begin(), SEQ_ACK_LENGTH-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), SEQ_ACK_LENGTH-acknowledge.size(), '0');
  messageEnd.insert(messageEnd.begin(), MESSAGE_END_LENGTH-messageEnd.size(), '0');
  return DAT + sequence + acknowledge + messageEnd + data;
}

bool receiveSynPacket(int& seq, int& ack, int& maxBuffer, int& maxMessage, std::string& buffer) {
  ack = addSeqAckNumber(atoi(buffer.substr(PACKET_TYPE_LENGTH, SEQ_ACK_LENGTH).data()), 1);
  if(buffer[0] == SYN) {
    if(atoi(buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH, BUFFER_SIZE_LENGTH).data()) != maxBuffer) {
      maxBuffer = std::min(maxBuffer, atoi(buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH, BUFFER_SIZE_LENGTH).data()));
    }
    if(atoi(buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH, MESSAGE_SIZE_LENGTH).data()) != maxMessage) {
      maxMessage = std::min(maxMessage, atoi(buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH, MESSAGE_SIZE_LENGTH).data()));
    }
    return true;
  }
  else
    return false;
}

bool receiveSynAckPacket(int& seq, int& ack, int& maxBuffer, int& maxMessage, std::string& buffer) {
  ack = addSeqAckNumber(atoi(buffer.substr(PACKET_TYPE_LENGTH, SEQ_ACK_LENGTH).data()), 1);
  seq = addSeqAckNumber(seq, 1);
  if(buffer[0] == SYNACK) {
    if(atoi(buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH, SEQ_ACK_LENGTH).data()) == seq) {
      if(atoi(buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), BUFFER_SIZE_LENGTH).data()) != maxBuffer) {
        maxBuffer = std::min(maxBuffer, atoi(buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), BUFFER_SIZE_LENGTH).data()));
      }
      if(atoi(buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH, MESSAGE_SIZE_LENGTH).data()) != maxMessage) {
        maxMessage = std::min(maxMessage, atoi(buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH, MESSAGE_SIZE_LENGTH).data()));
      }
    }
    else
      return false;
    return true;
  }
  else
    return false;
}

bool receiveAckPacket(int& seq, int& ack, std::string& buffer) {
  seq = addSeqAckNumber(seq, 1);
  if(buffer[0] == ACK) {
    if(atoi(buffer.substr(PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH, SEQ_ACK_LENGTH).data()) == seq) {
      ack = addSeqAckNumber(atoi(buffer.substr(PACKET_TYPE_LENGTH, SEQ_ACK_LENGTH).data()), 1);
    }
    else
      return false;
    return true;
  }
  else
    return false;
}

int receiveDatPacket(std::string& buffer, std::string& fullMessage) {
  if(buffer[0] != DAT)
    return atoi(buffer.substr(0,PACKET_TYPE_LENGTH).data());
  int messageLengthBefore = fullMessage.size();
  int messageEnd = atoi(buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), MESSAGE_END_LENGTH).data());
  fullMessage +=
  buffer.substr(PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+MESSAGE_END_LENGTH, messageEnd);
  if(fullMessage.size() == messageLengthBefore+messageEnd)
    return 0;
  else
    return -1;
}

void sendRstPacket(int socket, std::string sendStr, int seq, int ack) {
  sendStr = createCtrlPacket(RST, seq, ack);
  send(socket, sendStr.data(), PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), 0);
  std::cout << "Sent: " << std::endl;
  printPacket(sendStr);
  return;
}
