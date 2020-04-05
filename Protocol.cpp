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

void sendPacket(int socket, Packet& packet)
{
  std::string sendStr = packet.toString();
  send(socket, sendStr.data(), packet.getSize(), 0);
  std::cout << "Sent: " << sendStr << std::endl;
  packet.printPacket();
  std::cout << "----------------------------------\n";
}

char recvPacket(int socket, char* buffer, int bufferSize, Packet& packet)
{
  int valread = recv(socket, buffer, PACKET_TYPE_LENGTH, 0);
  if(valread > 0)
  {
    packet.setType(buffer[0]); //Also sets size
    valread = recv(socket, buffer, packet.getSize(), 0);
    packet.setPacket(buffer);
    if(packet.getType() == DAT || packet.getType() == REQ || packet.getType() == RAK)
    {
      valread = recv(socket, buffer, packet.getField2(), 0);
      packet.setData(buffer);
    }
    std::cout << "Received: \n";
    packet.printPacket();
    std::cout << "----------------------------------\n";
    return packet.getType();
  }
  if(valread == 0)
    return 'e'; //Empty string
  else
    return 't'; //Timeout
}
