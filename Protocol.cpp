#include "Protocol.h"
#include <sys/socket.h>
#include <string>
#include <iostream>

int addSeqAckNumber(int target, int num)
{
  target += num;
  if(target > 999999)
    target -= 1000000;

  return target;
}

bool checkAck(int& seq, int& ack, Packet& packet)
{
  if(packet.getType() != ACK || packet.getAck() != seq)
    return false;
  else
    return true;
}

void sendPacket(int socket, Packet& packet)
{
  std::string sendStr = packet.toString();
  send(socket, sendStr.data(), packet.getSize(), 0);
  std::cout << "Sent: " << sendStr << std::endl;
  packet.printPacket();
}

int recvPacket(int socket, char* buffer, int bufferSize, Packet& packet)
{
  int valread = recv(socket, buffer, bufferSize, 0);
  if(valread)
  {
    std::cout << "Receive buffer: " << buffer << std::endl;
    packet.setPacket(buffer);
    packet.printPacket();
    return packet.getType();
  }
  return -1;
}
