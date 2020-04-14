#include "Protocol.h"
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>

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

char recvPacket(int socket, char* buffer, Packet& packet)
{
  int valread = recv(socket, buffer, PACKET_TYPE_LENGTH, 0);
  if(valread > 0)
  {
    packet.setType(buffer[0]); //Also sets size
    valread = recv(socket, buffer, packet.getSize()-PACKET_TYPE_LENGTH, 0);
    packet.setPacket(buffer);
    if(packet.getType() == DAT || packet.getType() == REQ || packet.getType() == RAK)
    {
      int totalRead = 0;
      while(totalRead < packet.getField2())
      {
        valread = recv(socket, buffer+totalRead, packet.getField2()-totalRead, 0);
        totalRead += valread;
      }
      packet.setData(buffer);
    }
    packet.printPacket();
    std::cout << "----------------------------------\n";
    return packet.getType();
  }
  if(valread == 0)
    return 'e'; //Empty string
  else
    return 't'; //Timeout
}

int handleSynPacket(int socket, Packet& packet, Session& session, char packetType)
{
  if(packetType == 't')
  {
    packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 98);
    sendPacket(socket, packet);
    return 98;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 97);
    sendPacket(socket, packet);
    return 97;
  }
  //Process SYN packet and make a SAK packet
  session.acknowledgeNumber = addSeqAckNumber(packet.getSeq(), 1);
  if(packetType != SYN)
  {
    packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 1);
    sendPacket(socket, packet);
    return 1;
  }
  session.maxBuffer = std::min(session.maxBuffer, packet.getField1());
  session.maxMessage = std::min(session.maxMessage, packet.getField2());
  //Send SAK packet
  packet.setPacket(
    SAK,
    session.sequenceNumber,
    session.acknowledgeNumber,
    session.maxBuffer,
    session.maxMessage
  );
  sendPacket(socket, packet);
  session.sequenceNumber = addSeqAckNumber(session.sequenceNumber, 1);
  session.maxBuffer = static_cast<int>(pow(2, session.maxBuffer));
  session.maxMessage = static_cast<int>(pow(2, session.maxMessage));
  session.buffer.reserve(session.maxBuffer);
  return 0;
}

int handleAckPacket(int socket, Packet& packet, Session& session, char packetType, bool first)
{
  if(packetType == 't')
  {
    packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 98);
    sendPacket(socket, packet);
    return 98;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 97);
    sendPacket(socket, packet);
    return 97;
  }
  if(first)
    session.acknowledgeNumber = addSeqAckNumber(session.acknowledgeNumber, 1);
  else
    session.acknowledgeNumber = addSeqAckNumber(session.acknowledgeNumber, packet.getSize());
  //Check ACK packet
  if(packetType != ACK)
  {
    packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 3);
    sendPacket(socket, packet);
    return 3;
  }
  if(packet.getAck() != session.sequenceNumber)
  {
    packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 99);
    sendPacket(socket, packet);
    return 99;
  }
  return 0;
}

int handleReqPacket(int socket, Packet& packet, Session& session)
{
  if(packet.getAck() != session.sequenceNumber)
  {
    packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 99);
    sendPacket(socket, packet);
    return 99;
  }
  std::string filename = packet.getData();
  if(packet.getField1() == 0) //Read request
  {
    std::ifstream readFile("./files/"+filename, std::ios::in | std::ios::ate | std::ios::binary);
    if(!readFile.is_open())
    {
      packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 5);
      sendPacket(socket, packet);
      return 5;
    }
    std::streampos fileSize = readFile.tellg();
    if(fileSize > session.maxMessage) {
      packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 6);
      sendPacket(socket, packet);
      return 6;
    }
    readFile.seekg(0, std::ios::beg);
    packet.setPacket(
      RAK,
      session.sequenceNumber,
      session.acknowledgeNumber,
      fileSize,
      filename.size(),
      filename
    );
    sendPacket(socket, packet);
    session.sequenceNumber = addSeqAckNumber(session.sequenceNumber, packet.getSize());
    //Receive ACK packet
    int packetType = recvPacket(socket, session.buffer.data(), packet);
    int error;
    if((error = handleAckPacket(socket, packet, session, packetType)) != 0)
    {
      return error;
    }
    //REQ handshake complete
    int chunkSize = session.maxBuffer - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+DATA_LENGTH_LENGTH);
    int chunkTotal = fileSize / chunkSize;
    if(fileSize % chunkSize)
      chunkTotal++;
    std::cout << "****Number of chunks: " << chunkTotal << "****\n";
    std::cout << "****Chunk size: " << chunkSize << "****\n";
    int chunkCounter = 0;
    std::vector<char> fileBuffer(chunkSize);
    while(readFile.tellg() < fileSize)
    {
      if(!(fileSize-readFile.tellg() > chunkSize))
        chunkSize = fileSize-readFile.tellg();
      readFile.read(fileBuffer.data(), chunkSize);
      packet.setPacket(
        DAT,
        session.sequenceNumber,
        session.acknowledgeNumber,
        chunkCounter++,
        chunkSize,
        std::string(fileBuffer.data(), chunkSize)
      );
      sendPacket(socket, packet);
      session.sequenceNumber = addSeqAckNumber(session.sequenceNumber, packet.getSize());
    }
    readFile.close();
    packet.setPacket(DON, session.sequenceNumber, session.acknowledgeNumber);
    sendPacket(socket, packet);
    session.sequenceNumber = addSeqAckNumber(session.sequenceNumber, packet.getSize());
  }
  else //Write request
  {
    int chunkSize, chunkTotal;
    chunkSize = session.maxBuffer - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+DATA_LENGTH_LENGTH);
    chunkTotal = packet.getField1() / chunkSize;
    if(packet.getField1() % chunkSize)
      chunkTotal++;
    std::cout << "****Number of chunks: " << chunkTotal << "****\n";
    std::cout << "****Chunk size: " << chunkSize << "****\n";
    std::ofstream writeFile;
    writeFile.open("./files/"+filename);
    if(!writeFile.is_open())
    {
      packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 9);
      sendPacket(socket, packet);
      return 9;
    }
    packet.setPacket(
      RAK,
      session.sequenceNumber,
      session.acknowledgeNumber,
      packet.getField1(),
      filename.size(),
      filename
    );
    sendPacket(socket, packet);
    session.sequenceNumber = addSeqAckNumber(session.sequenceNumber, packet.getSize());
    int packetType = recvPacket(socket, session.buffer.data(), packet);
    while(packetType == DAT)
    {
      //Assuming no dropped packets
      session.acknowledgeNumber = packet.getSeq() + packet.getSize();
      writeFile.write(packet.getData().data(), packet.getField2());
      packetType = recvPacket(socket, session.buffer.data(), packet);
    }
    if(packetType == 't')
    {
      packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 98);
      sendPacket(socket, packet);
      return 98;
    }
    if(packetType == 'e')
    {
      packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 97);
      sendPacket(socket, packet);
      return 97;
    }
    //Assuming no dropped packets
    session.acknowledgeNumber = packet.getSeq() + packet.getSize();
    if(packet.getType() != DON)
    {
      packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 10);
      sendPacket(socket, packet);
      return 10;
    }
    if(packet.getAck() != session.sequenceNumber)
    {
      packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 99);
      sendPacket(socket, packet);
      return 99;
    }
    writeFile.close();
    packet.setPacket(DON, session.sequenceNumber, session.acknowledgeNumber);
    sendPacket(socket, packet);
    session.sequenceNumber = addSeqAckNumber( session.sequenceNumber, packet.getSize());
  }
  return 0;
}

int handleFinPacket(int socket, Packet& packet, Session& session)
{
  if(packet.getAck() != session.sequenceNumber)
  {
    packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 99);
    sendPacket(socket, packet);
    return 99;
  }
  packet.setPacket(FAK, session.sequenceNumber, session.acknowledgeNumber);
  sendPacket(socket, packet);
  session.sequenceNumber = addSeqAckNumber(session.sequenceNumber, packet.getSize());
  int packetType = recvPacket(socket, session.buffer.data(), packet);
  int error;
  if((error = handleAckPacket(socket, packet, session, packetType)) != 0)
  {
    return error;
  }
  return 0;
}

void handleRstPacket(int socket, Packet& packet, Session& session)
{
  std::cerr << RSTERRORS.at(packet.getField1()) << std::endl;
  packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 96);
  std::cout << RSTERRORS.at(96) << std::endl;
  sendPacket(socket, packet);
}
