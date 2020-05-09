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

using namespace std;

int main(int argc, char const *argv[])
{
  int error = 0;
  if(argc < 4)
  {
    error = 101;
    cerr << "Error " << error << ": Please enter valid arguments:\n"
         << "r <filename> <port> [max buffer] [max message]\n"
         << "w <filename> <port> [max buffer] [max message]\n"
         << "u \"+:filename:block:filesize\" <port> [max buffer] [max message]"
         << endl;
    return error;
  }
  if(argv[1][0] != 'r' && argv[1][0] != 'w' && argv[1][0] != 'u')
  {
    error = 102;
    cerr << "Error " << error << ": Please enter 'r' for read, 'w' for write, or 'u' for update" << endl;
    return error;
  }
  if(argc > 4)
  {
    if(atoi(argv[4]) < 5)
    {
      //Buffer size < 32B
      error = 103;
      cerr << "Error " << error << ": Buffer too small" << endl;
      return error;
    }
    if(atoi(argv[4]) > 25)
    {
      //Buffer size > ~33MB
      error = 104;
      cerr << "Error " << error << ": Buffer too big" << endl;
      return error;
    }
    if(argc > 5)
    {
      if(atoi(argv[5]) > 30)
      {
        //Max message size > 1GB
        error = 105;
        cerr << "Error " << error << ": Max message size too big" << endl;
        return error;
      }
    }
  }
  int sock = 0;
  struct sockaddr_in serv_addr;
  short PORT = atoi(argv[3]);

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    error = 106;
    cerr << "Error " << error << ": Socket creation error \n";
    return error;
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
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
  {
    error = 107;
    cerr << "Error " << error << ": Invalid address/ Address not supported \n";
    return error;
  }

  if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    error = 108;
    cerr << "Error " << error << ": Connection failed \n";
    return error;
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
    cout << "Error 98: " << RSTERRORS.at(98) << endl;
    return 98;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
    sendPacket(sock, packet);
    cout << "Error 97: " << RSTERRORS.at(97) << endl;
    return 97;
  }
  //Process SAK packet and make an ACK packet
  acknowledgeNumber = addSeqAckNumber(packet.getSeq(), 1);
  if(packetType != SAK)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 2);
    sendPacket(sock, packet);
    cout << "Error 2: " << RSTERRORS.at(2) << endl;
    return 2;
  }
  if(packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
    sendPacket(sock, packet);
    cout << "Error 99: " << RSTERRORS.at(99) << endl;
    return 99;
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
    if(filename.size() > (bufferVal - (PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+DATA_LENGTH_LENGTH)))
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 4);
      sendPacket(sock, packet);
      cout << "Error 4: " << RSTERRORS.at(4) << endl;
      return 4;
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
      cout << "Error 98: " << RSTERRORS.at(98) << endl;
      return 98;
    }
    if(packetType == 'e')
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
      sendPacket(sock, packet);
      cout << "Error 97: " << RSTERRORS.at(97) << endl;
      return 97;
    }
    acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
    //Check and send response to RAK
    if(packet.getType() != RAK)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 7);
      sendPacket(sock, packet);
      cout << "Error 7: " << RSTERRORS.at(7) << endl;
      return 7;
    }
    if(packet.getAck() != sequenceNumber)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
      sendPacket(sock, packet);
      cout << "Error 99: " << RSTERRORS.at(99) << endl;
      return 99;
    }
    if(packet.getData() != filename)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 8);
      sendPacket(sock, packet);
      cout << "Error 8: " << RSTERRORS.at(8) << endl;
      return 8;
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
      cout << "Error 9: " << RSTERRORS.at(9) << endl;
      return 9;
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
  else if(argv[1][0] == 'w')
  {
    ifstream readFile("./files/"+filename, ios::in | ios::ate | ios::binary);
    if(!readFile.is_open())
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 5);
      sendPacket(sock, packet);
      cout << "Error 5: " << RSTERRORS.at(5) << endl;
      return 5;
    }
    streampos fileSize = readFile.tellg();
    if(fileSize > maxMessageVal) {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 6);
      sendPacket(sock, packet);
      cout << "Error 6: " << RSTERRORS.at(6) << endl;
      return 6;
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
      cout << "Error 98: " << RSTERRORS.at(98) << endl;
      return 98;
    }
    if(packetType == 'e')
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
      sendPacket(sock, packet);
      cout << "Error 97: " << RSTERRORS.at(97) << endl;
      return 97;
    }
    acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
    //Check RAK
    if(packet.getType() != RAK)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 7);
      sendPacket(sock, packet);
      cout << "Error 7: " << RSTERRORS.at(7) << endl;
      return 7;
    }
    if(packet.getAck() != sequenceNumber)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
      sendPacket(sock, packet);
      cout << "Error 99: " << RSTERRORS.at(99) << endl;
      return 99;
    }
    if(packet.getData() != filename)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 8);
      sendPacket(sock, packet);
      cout << "Error 8: " << RSTERRORS.at(8) << endl;
      return 8;
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
  else if(argv[1][0] == 'u')
  {
    if(filename.size()+PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+FILE_SIZE_LENGTH+DATA_LENGTH_LENGTH > bufferVal)
    {
      packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 4);
      sendPacket(sock, packet);
      cout << "Error 4: " << RSTERRORS.at(4) << endl;
      return 4;
    }
    packet.setPacket(
      REQ,
      sequenceNumber,
      acknowledgeNumber,
      -1,
      filename.size(),
      filename
    );
    sendPacket(sock, packet);
    sequenceNumber = addSeqAckNumber(sequenceNumber, packet.getSize());
    packetType = recvPacket(sock, buffer.data(), packet);
  }
  if(packetType == 't')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 98);
    sendPacket(sock, packet);
    cout << "Error 98: " << RSTERRORS.at(98) << endl;
    return 98;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
    sendPacket(sock, packet);
    cout << "Error 97: " << RSTERRORS.at(97) << endl;
    return 97;
  }
  acknowledgeNumber = packet.getSeq() + packet.getSize();
  if(packet.getType() != DON)
  {
    if(packet.getType() == RST && packet.getField1() == 9)
      error = 9;
    else error = 10;
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, error);
    sendPacket(sock, packet);
    cout << "Error " << error << ": " << RSTERRORS.at(error) << endl;
    return error;
  }
  if(packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
    sendPacket(sock, packet);
    cout << "Error 99: " << RSTERRORS.at(99) << endl;
    return 99;
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
    cout << "Error 98: " << RSTERRORS.at(98) << endl;
    return 98;
  }
  if(packetType == 'e')
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 97);
    sendPacket(sock, packet);
    cout << "Error 97: " << RSTERRORS.at(97) << endl;
    return 97;
  }
  acknowledgeNumber = addSeqAckNumber(acknowledgeNumber, packet.getSize());
  if(packet.getType() != FAK)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 11);
    sendPacket(sock, packet);
    cout << "Error 11: " << RSTERRORS.at(11) << endl;
    return 11;
  }
  if(packet.getAck() != sequenceNumber)
  {
    packet.setPacket(RST, sequenceNumber, acknowledgeNumber, 99);
    sendPacket(sock, packet);
    cout << "Error 99: " << RSTERRORS.at(99) << endl;
    return 99;
  }
  //Send ACK
  packet.setPacket(ACK, sequenceNumber, acknowledgeNumber);
  sendPacket(sock, packet);
  return 0;
}
