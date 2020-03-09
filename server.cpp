#include <iostream>
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
#define PORT 8080

using namespace std;

int main(int argc, char const *argv[]) {
  if(argc > 1) {
    if(atoi(argv[1]) < 5) {
      //Buffer size < 32B
      cerr << "Buffer too small" << endl;
      return 1;
    }
    if(atoi(argv[1]) > 26) {
      //Buffer size > ~66MB
      cerr << "Buffer too big" << endl;
      return 1;
    }
    if(atoi(argv[2]) > 31) {
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
  char* buffer = new char[32];
  srand(time(NULL)+1);
  int sequenceNumber = rand() % 1000000; cout << "Server sequence number: " << sequenceNumber << endl;
  int acknowledgeNumber = 0;
  int maxBuffer = 5;
  int maxMessage = 10;
  if(argc > 1) {
    maxBuffer = atoi(argv[1]);
    maxMessage = atoi(argv[2]);
  }
  //Read SYN packet
  valread = read(new_socket, buffer, PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH);
  string bufferStr(buffer, PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH);
  cout << "Received: " << endl;
  printPacket(bufferStr);
  //Check and process SYN packet
  string sendStr;
  if(!receiveSynPacket(sequenceNumber, acknowledgeNumber, maxBuffer, maxMessage, bufferStr)) {
    sendRstPacket(new_socket, sendStr, sequenceNumber, acknowledgeNumber);
    return -1;
  }
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  delete [] buffer;
  char* newBuffer = new char[bufferVal];
  buffer = newBuffer;
  //Send SYNACK packet
  sendStr = createSynAckPacket(sequenceNumber, acknowledgeNumber, maxBuffer, maxMessage);
  send(new_socket, sendStr.data(), PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH, 0);
  cout << "Sent: " << endl;
  printPacket(sendStr);
  //Reah ACK packet
  valread = read(new_socket, buffer, PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH));
  bufferStr = bufferToString(buffer, PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH));
  cout << "Received: " << endl;
  printPacket(bufferStr);
  //Check ACK packet
  if(!receiveAckPacket(sequenceNumber, acknowledgeNumber, bufferStr)) {
    sendRstPacket(new_socket, sendStr, sequenceNumber, acknowledgeNumber);
    return -1;
  }
  //Handshake complete
  string theMessage = "This message is 64 bytes long. THIS MESSAGE IS 64 BYTES LONG.";
  int packetMessageLength = bufferVal - DATA_HEADER_LENGTH;
  int messageStartIndex = 0;
  string packetMessage;
  while(messageStartIndex < theMessage.size()) {
    if(theMessage.size()-messageStartIndex < packetMessageLength) {
      packetMessageLength = theMessage.size() - messageStartIndex;
    }
    packetMessage = theMessage.substr(messageStartIndex, packetMessageLength);
    sendStr = createDatPacket(sequenceNumber, acknowledgeNumber, packetMessageLength, packetMessage);
    messageStartIndex += packetMessageLength;
    sequenceNumber = addSeqAckNumber(sequenceNumber, sendStr.size());
    send(new_socket, sendStr.data(), bufferVal, 0);
    cout << "Sent: " << endl;
    printPacket(sendStr);
  }

  return 0;
}
