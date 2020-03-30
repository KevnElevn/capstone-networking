#include <iostream>
#include <fstream>
//POSIX API
#include <unistd.h>
#include <sys/socket.h>
#include <cstdlib>
//Internet address family
#include <netinet/in.h>
#include <string>
#include <cmath>
#include <ctime>
#include "Protocol.h"
#include "Packet.h"
#define PORT 8080

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
    if(atoi(argv[2]) > 31)
    {
      //Max message size > 2GB
      cerr << "Max message size too big" << endl;
      return 2;
    }
  }

  int server_fd,  //socket descriptor
      new_socket, //used to accept connection request
      valread;
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
  char* initBuffer = new char[32];
  srand(time(NULL)+1);
  int sequenceNumber = rand() % 1000000; cout << "Server sequence number: " << sequenceNumber << endl;
  int acknowledgeNumber = 0;
  int maxBuffer = 10;
  int maxMessage = 10;
  if(argc > 1)
  {
    maxBuffer = atoi(argv[1]);
    maxMessage = atoi(argv[2]);
  }
  //Read SYN packet
  Packet packet;
  valread = recvPacket(new_socket, initBuffer, PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH, packet);
  //Process SYN packet and make a SAK packet
  acknowledgeNumber = addSeqAckNumber(packet.getSeq(), 1);
  if(packet.getType() != SYN)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(new_socket, packet);
    cerr << "Expected a SYN packet\n";
    return -1;
  }
  maxBuffer = min(maxBuffer, packet.getField1());
  maxMessage = min(maxMessage, packet.getField2());
  packet.setPacket(
    SAK,
    sequenceNumber,
    acknowledgeNumber,
    maxBuffer,
    maxMessage
  );
  sequenceNumber = addSeqAckNumber(sequenceNumber, 1);
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  delete [] initBuffer;
  char* buffer = new char[bufferVal];
  //Send SAK packet
  sendPacket(new_socket, packet);
  //Read ACK packet
  valread = recvPacket(new_socket, buffer, PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), packet);
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, 1);
  //Check ACK packet
  if(!checkAck(sequenceNumber, acknowledgeNumber, packet)) {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(new_socket, packet);
    cerr << "Expected an ACK packet with correct sequence and acknowledge numbers\n";
    return -1;
  }
  //Inital handshake complete
  //Read REQ packet
  valread = recvPacket(new_socket, buffer, bufferVal, packet);
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
  if(packet.getType() != REQ || packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(new_socket, packet);
    cerr << "Expected REQ packet with correct sequence and acknowledge numbers\n";
    return -1;
  }
  string filename = packet.getData();
  ifstream readFile("input/"+filename, ios::in | ios::ate | ios::binary);
  if(!readFile.is_open())
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(new_socket, packet);
    cerr << "File not found\n";
    return -1;
  }
  streampos fileSize = readFile.tellg();
  if(fileSize > maxMessageVal) {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(new_socket, packet);
    cerr << "File is too large for current MaxMessage value\n";
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
  valread = recvPacket(new_socket, buffer, bufferVal, packet);
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
  //Check ACK packet
  if(!checkAck(sequenceNumber, acknowledgeNumber, packet))
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(new_socket, packet);
    return -1;
  }
  //REQ handshake complete
  int chunkSize = bufferVal - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+MESSAGE_LENGTH);
  int chunkTotal = fileSize / chunkSize;
  if(fileSize % chunkSize)
    chunkTotal++;
  cout << "****Number of chunks: " << chunkTotal << "****\n";
  cout << "****Chunk size: " << chunkSize << "****\n";
  int pieceCounter = 0;
  string fstr;
  char * fileBuffer = new char[chunkSize];
  while(readFile.tellg()<fileSize)
  {
    if(!(fileSize-readFile.tellg() > chunkSize))
      chunkSize = fileSize-readFile.tellg();
    readFile.read(fileBuffer, chunkSize);
    packet.setPacket(
      DAT,
      sequenceNumber,
      acknowledgeNumber,
      pieceCounter++,
      chunkSize,
      string(fileBuffer, chunkSize)
    );
    sendPacket(new_socket, packet);
    sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
  }
  packet.setPacket(DON, sequenceNumber, acknowledgeNumber);
  sendPacket(new_socket, packet);
  sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
  //Assuming everything is good, FIN handshake
  valread = recvPacket(new_socket, buffer, bufferVal, packet);
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
  if(packet.getType() != FIN || packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(new_socket, packet);
    return -1;
  }
  packet.setPacket(FAK, sequenceNumber, acknowledgeNumber);
  sendPacket(new_socket, packet);
  sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
  valread = recvPacket(new_socket, buffer, bufferVal, packet);
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
  if(!checkAck(sequenceNumber, acknowledgeNumber, packet))
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(new_socket, packet);
    return -1;
  }

  readFile.close();
  delete[] fileBuffer;
  delete[] buffer;

  return 0;
}
