#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstdlib>
#include <cmath>
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
    if(atoi(argv[1]) > 25)
    {
      //Buffer size > ~33MB
      cerr << "Buffer too big" << endl;
      return 1;
    }
    if(atoi(argv[2]) > 30)
    {
      //Max message size > 1GB
      cerr << "Max message size too big" << endl;
      return 2;
    }

  }
  int sock = 0, valread;
  struct sockaddr_in serv_addr;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    cerr << "Socket creation error \n";
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form (Network byte order)
  //sin_port and sin_addr must be in network byte order
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
  {
    cerr << "Invalid address/ Address not supported \n";
    return -1;
  }

  if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    cerr << "Connection failed \n";
    return -1;
  }
  //Connected
  srand(time(NULL));
  char* initBuffer = new char[32];
  int sequenceNumber = rand() % 1000000;
  int acknowledgeNumber = 0;
  int maxBuffer = 6;
  int maxMessage = 10;
  if(argc > 1)
  {
    maxBuffer = atoi(argv[1]);
    maxMessage = atoi(argv[2]);
  }
  //Send SYN packet
  Packet packet(
    SYN,
    sequenceNumber,
    acknowledgeNumber,
    maxBuffer,
    maxMessage
  );
  sendPacket(sock, packet);
  sequenceNumber = addSeqAckNumber(sequenceNumber, 1);
  //Read SAK packet
  valread = recvPacket(sock, initBuffer, PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH, packet);
  //Process SAK packet and make an ACK packet
  acknowledgeNumber = addSeqAckNumber(packet.getSeq(), 1);
  if(packet.getType() != SAK || packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(sock, packet);;
    cerr << "Expected a SAK packet\n";
    return -1;
  }
  maxBuffer = min(maxBuffer, packet.getField1());
  maxMessage = min(maxMessage, packet.getField2());
  packet.setPacket(ACK, sequenceNumber, acknowledgeNumber);
  sequenceNumber = addSeqAckNumber(sequenceNumber, 1);
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  delete [] initBuffer;
  char* buffer = new char[bufferVal];
  sendPacket(sock, packet);
  //Initial handshake complete
  //Send REQ packet
  string filename = "file.txt";
  if(filename.size() > (bufferVal - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+MESSAGE_LENGTH)))
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(sock, packet);;
    cerr << "Filename too long to send with current buffer size. Try again with a bigger buffer.\n";
    return -1;
  }
  packet.setPacket(
    REQ,
    sequenceNumber,
    acknowledgeNumber,
    filename.size(),
    0,
    filename
  );
  sendPacket(sock, packet);
  sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
  //Receive RAK packet
  valread = recvPacket(sock, buffer, bufferVal, packet);
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
  //Check and send response to RAK
  if(packet.getType() != RAK || packet.getAck() != sequenceNumber || packet.getData() != filename)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(sock, packet);;
    cerr << "Expected a RAK packet\n";
    return -1;
  }
  int chunkSize = bufferVal - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+MESSAGE_LENGTH);
  int chunkTotal = packet.getField1() / chunkSize;
  if(packet.getField1() % chunkSize)
    chunkTotal++;
  cout << "****Number of chunks: " << chunkTotal << "****\n";
  cout << "****Chunk size: " << chunkSize << "****\n";
  packet.setPacket(ACK, sequenceNumber, acknowledgeNumber);
  sendPacket(sock, packet);
  sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
  //Receive DAT packets
  ofstream writeFile("output/"+filename);
  if(!writeFile.is_open())
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(sock, packet);
    cerr << "Unable to open output/" << filename << endl;
    return -1;
  }
  while(recvPacket(sock, buffer, bufferVal, packet) == DAT)
  {
    //Assuming no dropped packets
    acknowledgeNumber = packet.getSeq() + packet.getSize();
    writeFile.write(packet.getData().data(), packet.getField2());
  }
  acknowledgeNumber = packet.getSeq() + packet.getSize();
  if(packet.getType() != DON || packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(sock, packet);
    return -1;
  }
  //If everything is good, send FIN
  packet.setPacket(FIN, sequenceNumber, acknowledgeNumber);
  sendPacket(sock, packet);
  sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
  //Receive FAK
  valread = recvPacket(sock, buffer, bufferVal, packet);
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
  if(packet.getType() != FAK || packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber);
    sendPacket(sock, packet);
    return -1;
  }
  //Send ACK
  packet.setPacket(ACK, sequenceNumber, acknowledgeNumber);
  sendPacket(sock, packet);

  writeFile.close();
  delete[] buffer;

  return 0;
}
