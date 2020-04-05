#ifndef PACKET_H
#define PACKET_H
#include <string>

class Packet
{
private:
  int size;
  char type;
  int sequence;
  int acknowledge;
  int field1;
  int field2;
  std::string data;

public:
  Packet();
  Packet(char packetType, int seq, int ack, int f1 = 0, int f2 = 0, std::string dat = "");
  Packet(char* buffer);

  void setPacket(char packetType, int seq, int ack, int f1 = 0, int f2 = 0, std::string dat = "");
  void setPacket(char* buffer);
  void setType(char t);
  void setData(char* buffer);
  void printPacket();
  std::string toString() const;

  int getSize() { return size; }
  char getType() { return type; }
  int getSeq() { return sequence; }
  int getAck() { return acknowledge; }
  int getField1() { return field1; }
  int getField2() { return field2; }
  std::string getData() { return data; }
};

#endif
