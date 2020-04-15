#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include "Protocol.h"
#include "Packet.h"
#define PORT 7253

using namespace std;

int main(int argc, char const *argv[])
{
  if(argc < 4)
  {
    cerr << "Please enter 'r' for read or 'w' for write, followed by a file name to read from or write to, and an IP address to connect to." << endl;
    return 1;
  }
  else
  {
    if(argv[1][0] != 'r' && argv[1][0] != 'w')
    {
      cerr << "Please enter 'r' for read or 'w' for write" << endl;
      return 1;
    }
  }
  if(argc > 4)
  {
    if(atoi(argv[4]) < 5)
    {
      //Buffer size < 32B
      cerr << "Buffer too small" << endl;
      return 1;
    }
    if(atoi(argv[4]) > 25)
    {
      //Buffer size > ~33MB
      cerr << "Buffer too big" << endl;
      return 1;
    }
    if(argc > 5)
    {
      if(atoi(argv[5]) > 30)
      {
        //Max message size > 1GB
        cerr << "Max message size too big" << endl;
        return 2;
      }
    }
  }
  int sock = 0;
  struct sockaddr_in serv_addr;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    cerr << "Socket creation error \n";
    return -1;
  }

  //Set recv timeout for socket
  // struct timeval tv;
  // tv.tv_sec = 5; //5 second timeout
  // tv.tv_usec = 0;
  // if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)& tv, sizeof tv))
  // {
  //   cerr << "setsockopt failed\n";
  //   return -1;
  // }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form (Network byte order)
  //sin_port and sin_addr must be in network byte order
  if(inet_pton(AF_INET, argv[3], &serv_addr.sin_addr)<=0)
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
  cout << "Connected\n";
  srand(time(NULL));
  vector<char> buffer;
  buffer.reserve(32);
  int sequenceNumber = rand() % 1000000;
  int acknowledgeNumber = 0;
  int maxBuffer = 10;
  int maxMessage = 18;
  if(argc > 4)
  {
    maxBuffer = atoi(argv[4]);
    if(argc > 5)
      maxMessage = atoi(argv[5]);
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
  char packetType = recvPacket(sock, buffer.data(), packet);
  if(packetType == 't')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
    sendPacket(sock, packet);
    return -1;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
    sendPacket(sock, packet);
    return -1;
  }
  //Process SAK packet and make an ACK packet
  acknowledgeNumber = addSeqAckNumber(packet.getSeq(), 1);
  if(packetType != SAK)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 2);
    sendPacket(sock, packet);
    return -1;
  }
  if(packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
    sendPacket(sock, packet);
    return -1;
  }
  maxBuffer = min(maxBuffer, packet.getField1());
  maxMessage = min(maxMessage, packet.getField2());
  packet.setPacket(ACK, sequenceNumber, acknowledgeNumber);
  sendPacket(sock, packet);
  sequenceNumber = addSeqAckNumber(sequenceNumber, 1);
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  buffer.reserve(bufferVal);
  //Initial handshake complete

  //Send REQ packet
  string filename = argv[2];
  if(argv[1][0] == 'r')
  {
    // if(filename.find('/') != string::npos)
    // {
    //   packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 13);
    //   sendPacket(sock, packet);
    //   return -1;
    // }
    if(filename.size() > (bufferVal - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+DATA_LENGTH_LENGTH)))
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 4);
      sendPacket(sock, packet);
      return -1;
    }
    packet.setPacket(
      REQ,
      sequenceNumber,
      acknowledgeNumber,
      0,
      filename.size(),
      filename
    );
    sendPacket(sock, packet);
    sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
    //Receive RAK packet
    packetType = recvPacket(sock, buffer.data(), packet);
    if(packetType == 't')
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
      sendPacket(sock, packet);
      return -1;
    }
    if(packetType == 'e')
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
      sendPacket(sock, packet);
      return -1;
    }
    acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
    //Check and send response to RAK
    if(packet.getType() != RAK)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 7);
      sendPacket(sock, packet);
      return -1;
    }
    if(packet.getAck() != sequenceNumber)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
      sendPacket(sock, packet);
      return -1;
    }
    if(packet.getData() != filename)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 8);
      sendPacket(sock, packet);
      return -1;
    }
    int chunkSize, chunkTotal;
    chunkSize = bufferVal - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+DATA_LENGTH_LENGTH);
    chunkTotal = packet.getField1() / chunkSize;
    if(packet.getField1() % chunkSize)
      chunkTotal++;
    cout << "****Number of chunks: " << chunkTotal << "****\n";
    cout << "****Chunk size: " << chunkSize << "****\n";
    ofstream writeFile;
    writeFile.open("./files/"+filename);
    if(!writeFile.is_open())
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 9);
      sendPacket(sock, packet);
      return -1;
    }
    packet.setPacket(ACK, sequenceNumber, acknowledgeNumber);
    sendPacket(sock, packet);
    sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
    //Receive DAT packets
    packetType = recvPacket(sock, buffer.data(), packet);
    while(packetType == DAT)
    {
      //Assuming no dropped packets
      acknowledgeNumber = packet.getSeq() + packet.getSize();
      writeFile.write(packet.getData().data(), packet.getField2());
      packetType = recvPacket(sock, buffer.data(), packet);
    }
    writeFile.close();
  }
  else
  {
    ifstream readFile("./files/"+filename, ios::in | ios::ate | ios::binary);
    if(!readFile.is_open())
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 5);
      sendPacket(sock, packet);
      return -1;
    }
    streampos fileSize = readFile.tellg();
    if(fileSize > maxMessageVal) {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 6);
      sendPacket(sock, packet);
      return -1;
    }
    readFile.seekg(0, ios::beg);
    packet.setPacket(
      REQ,
      sequenceNumber,
      acknowledgeNumber,
      fileSize,
      filename.size(),
      filename
    );
    sendPacket(sock, packet);
    sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
    //Receive RAK packet
    packetType = recvPacket(sock, buffer.data(), packet);
    if(packetType == 't')
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
      sendPacket(sock, packet);
      return -1;
    }
    if(packetType == 'e')
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
      sendPacket(sock, packet);
      return -1;
    }
    acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
    //Check RAK
    if(packet.getType() != RAK)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 7);
      sendPacket(sock, packet);
      return -1;
    }
    if(packet.getAck() != sequenceNumber)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
      sendPacket(sock, packet);
      return -1;
    }
    if(packet.getData() != filename)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 8);
      sendPacket(sock, packet);
      return -1;
    }
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
      sendPacket(sock, packet);
      sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
    }
    delete[] fileBuffer;
    readFile.close();
    packet.setPacket(DON, sequenceNumber, acknowledgeNumber);
    sendPacket(sock, packet);
    sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
    packetType = recvPacket(sock, buffer.data(), packet);
  }
  if(packetType == 't')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
    sendPacket(sock, packet);
    return -1;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
    sendPacket(sock, packet);
    return -1;
  }
  acknowledgeNumber = packet.getSeq() + packet.getSize();
  if(packet.getType() != DON)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 10);
    sendPacket(sock, packet);
    return -1;
  }
  if(packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
    sendPacket(sock, packet);
    return -1;
  }
  //If everything is good, send FIN
  packet.setPacket(FIN, sequenceNumber, acknowledgeNumber);
  sendPacket(sock, packet);
  sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
  //Receive FAK
  packetType = recvPacket(sock, buffer.data(), packet);
  if(packetType == 't')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
    sendPacket(sock, packet);
    return -1;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
    sendPacket(sock, packet);
    return -1;
  }
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
  if(packet.getType() != FAK)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 11);
    sendPacket(sock, packet);
    return -1;
  }
  if(packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
    sendPacket(sock, packet);
    return -1;
  }
  //Send ACK
  packet.setPacket(ACK, sequenceNumber, acknowledgeNumber);
  sendPacket(sock, packet);
  return 0;
}
