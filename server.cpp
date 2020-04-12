#include <iostream>
#include <fstream>
//POSIX API
#include <unistd.h>
#include <sys/socket.h>
#include <cstdlib>
//Internet address family
#include <netinet/in.h>
#include <string>
#include <vector>
#include <cmath>
#include <ctime>
#include "Protocol.h"
#include "Packet.h"
#define PORT 7253

using namespace std;

int main(int argc, char const *argv[])
{
  if(argc > 1)
  {
    if(atoi(argv[1]) < 5)
    {
      //Buffer size < 32B
      cerr << "Buffer too small" << endl;
      return 1;
    }
    if(atoi(argv[1]) > 26)
    {
      //Buffer size > ~66MB
      cerr << "Buffer too big" << endl;
      return 1;
    }
    if(argc > 2)
    {
      if(atoi(argv[2]) > 31)
      {
        //Max message size > 2GB
        cerr << "Max message size too big" << endl;
        return 2;
      }
    }
  }

  int server_fd,  //socket descriptor
      new_socket; //used to accept connection request
  struct sockaddr_in address;
  int opt = 1;    //used for setsockopt() to manipulate options for the socket sockfd
  int addrlen = sizeof(address);

  // Create socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  /*
  socket(domain, type, protocol)
  domain: integer, communication domain e.g., AF_INET (IPv4 protocol) , AF_INET6 (IPv6 protocol)
  type: communication type
      SOCK_STREAM: TCP (reliable, connection oriented)
      SOCK_DGRAM: UDP(unreliable, connectionless)
  protocol: Protocol value for IP, which is 0
  */
  {
      perror("socket failed");
      exit(EXIT_FAILURE);
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
  {
      perror("setsockopt");
      exit(EXIT_FAILURE);
  }
  //Set recv timeout for socket
  // struct timeval tv;
  // tv.tv_sec = 5; //5 second timeout
  // tv.tv_usec = 0;
  // if(setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)& tv, sizeof tv))
  // {
  //   perror("setsockopt");
  //   exit(EXIT_FAILURE);
  // }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( PORT );

  //Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
      perror("bind failed");
      exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0) //listen(socket descriptor, backlog)
  {
      perror("listen");
      exit(EXIT_FAILURE);
  }
  cout << "Listening..." << endl;
  if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
  {
      perror("accept");
      exit(EXIT_FAILURE);
  }
  //Connected and listening
  vector<char> buffer;
  buffer.reserve(32);
  srand(time(NULL)+1);
  int sequenceNumber = rand() % 1000000; cout << "Server sequence number: " << sequenceNumber << endl;
  int acknowledgeNumber = 0;
  int maxBuffer = 10;
  int maxMessage = 30;
  if(argc > 1)
  {
    maxBuffer = atoi(argv[1]);
    if(argc > 2)
      maxMessage = atoi(argv[2]);
  }
  //Read SYN packet
  Packet packet;
  char packetType = recvPacket(new_socket, buffer.data(), PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH, packet);
  if(packetType == 't')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
    sendPacket(new_socket, packet);
    return -1;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
    sendPacket(new_socket, packet);
    return -1;
  }
  //Process SYN packet and make a SAK packet
  acknowledgeNumber = addSeqAckNumber(packet.getSeq(), 1);
  if(packetType != SYN)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 0);
    sendPacket(new_socket, packet);
    return -1;
  }
  maxBuffer = min(maxBuffer, packet.getField1());
  maxMessage = min(maxMessage, packet.getField2());
  //Send SAK packet
  packet.setPacket(
    SAK,
    sequenceNumber,
    acknowledgeNumber,
    maxBuffer,
    maxMessage
  );
  sendPacket(new_socket, packet);
  sequenceNumber = addSeqAckNumber(sequenceNumber, 1);
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  buffer.reserve(bufferVal);
  //Read ACK packet
  packetType = recvPacket(new_socket, buffer.data(), PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), packet);
  if(packetType == 't')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
    sendPacket(new_socket, packet);
    return -1;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
    sendPacket(new_socket, packet);
    return -1;
  }
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, 1);
  //Check ACK packet
  if(packetType != ACK)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 3);
    sendPacket(new_socket, packet);
    return -1;
  }
  if(packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
    sendPacket(new_socket, packet);
    return -1;
  }
  //Inital handshake complete
  packetType = recvPacket(new_socket, buffer.data(), bufferVal, packet);
  while(packetType > 0)
  {
    acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
    switch(packet.getType())
    {
      case REQ:
      {
        if(packet.getAck() != sequenceNumber)
        {
          packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
          sendPacket(new_socket, packet);
          return -1;
        }
        string filename = packet.getData();
        if(packet.getField1() == 0)
        {
          ifstream readFile("files/"+filename, ios::in | ios::ate | ios::binary);
          if(!readFile.is_open())
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 5);
            sendPacket(new_socket, packet);
            return -1;
          }
          streampos fileSize = readFile.tellg();
          if(fileSize > maxMessageVal) {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 6);
            sendPacket(new_socket, packet);
            return -1;
          }
          readFile.seekg(0, ios::beg);
          packet.setPacket(
            RAK,
            sequenceNumber,
            acknowledgeNumber,
            fileSize,
            filename.size(),
            filename
          );
          sendPacket(new_socket, packet);
          sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
          //Receive ACK packet
          packetType = recvPacket(new_socket, buffer.data(), bufferVal, packet);
          if(packetType == 't')
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
            sendPacket(new_socket, packet);
            return -1;
          }
          if(packetType == 'e')
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
            sendPacket(new_socket, packet);
            return -1;
          }
          acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
          if(packetType != ACK)
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 3);
            sendPacket(new_socket, packet);
            return -1;
          }
          if(packet.getAck() != sequenceNumber)
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
            sendPacket(new_socket, packet);
            return -1;
          }
          //REQ handshake complete
          int chunkSize = bufferVal - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+DATA_LENGTH_LENGTH);
          int chunkTotal = fileSize / chunkSize;
          if(fileSize % chunkSize)
            chunkTotal++;
          cout << "****Number of chunks: " << chunkTotal << "****\n";
          cout << "****Chunk size: " << chunkSize << "****\n";
          int chunkCounter = 0;
          char * fileBuffer = new char[chunkSize];
          while(readFile.tellg() < fileSize)
          {
            if(!(fileSize-readFile.tellg() > chunkSize))
              chunkSize = fileSize-readFile.tellg();
            readFile.read(fileBuffer, chunkSize);
            packet.setPacket(
              DAT,
              sequenceNumber,
              acknowledgeNumber,
              chunkCounter++,
              chunkSize,
              string(fileBuffer, chunkSize)
            );
            sendPacket(new_socket, packet);
            sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
          }
          delete[] fileBuffer;
          readFile.close();
          packet.setPacket(DON, sequenceNumber, acknowledgeNumber);
          sendPacket(new_socket, packet);
          sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
        }
        else
        {
          int chunkSize, chunkTotal;
          chunkSize = bufferVal - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+DATA_LENGTH_LENGTH);
          chunkTotal = packet.getField1() / chunkSize;
          if(packet.getField1() % chunkSize)
            chunkTotal++;
          cout << "****Number of chunks: " << chunkTotal << "****\n";
          cout << "****Chunk size: " << chunkSize << "****\n";
          ofstream writeFile;
          writeFile.open("files/"+filename);
          if(!writeFile.is_open())
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 9);
            sendPacket(new_socket, packet);
            return -1;
          }
          packet.setPacket(
            RAK,
            sequenceNumber,
            acknowledgeNumber,
            packet.getField1(),
            filename.size(),
            filename
          );
          sendPacket(new_socket, packet);
          sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
          packetType = recvPacket(new_socket, buffer.data(), bufferVal, packet);
          while(packetType == DAT)
          {
            //Assuming no dropped packets
            acknowledgeNumber = packet.getSeq() + packet.getSize();
            writeFile.write(packet.getData().data(), packet.getField2());
            packetType = recvPacket(new_socket, buffer.data(), bufferVal, packet);
          }
          if(packetType == 't')
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
            sendPacket(new_socket, packet);
            return -1;
          }
          if(packetType == 'e')
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
            sendPacket(new_socket, packet);
            return -1;
          }
          //Assuming no dropped packets
          acknowledgeNumber = packet.getSeq() + packet.getSize();
          if(packet.getType() != DON)
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 10);
            sendPacket(new_socket, packet);
            return -1;
          }
          if(packet.getAck() != sequenceNumber)
          {
            packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
            sendPacket(new_socket, packet);
            return -1;
          }
          writeFile.close();
          packet.setPacket(DON, sequenceNumber, acknowledgeNumber);
          sendPacket(new_socket, packet);
          sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
        }
        break;
      }

      case FIN:
      {
        if(packet.getAck() != sequenceNumber)
        {
          packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
          sendPacket(new_socket, packet);
          return -1;
        }
        packet.setPacket(FAK, sequenceNumber, acknowledgeNumber);
        sendPacket(new_socket, packet);
        sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
        packetType = recvPacket(new_socket, buffer.data(), bufferVal, packet);
        if(packetType == 't')
        {
          packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
          sendPacket(new_socket, packet);
          return -1;
        }
        if(packetType == 'e')
        {
          packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
          sendPacket(new_socket, packet);
          return -1;
        }
        acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
        if(packetType != ACK)
        {
          packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 3);
          sendPacket(new_socket, packet);
          return -1;
        }
        if(packet.getAck() != sequenceNumber)
        {
          packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
          sendPacket(new_socket, packet);
          return -1;
        }
        return 0;
        break;
      }

      default:
      {
        packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 12);
        sendPacket(new_socket, packet);
        return -1;
      }
    }
    packetType = recvPacket(new_socket, buffer.data(), bufferVal, packet);
  }
  if(packetType == 't')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
    sendPacket(new_socket, packet);
    return -1;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
    sendPacket(new_socket, packet);
    return -1;
  }

  return -1;
}
