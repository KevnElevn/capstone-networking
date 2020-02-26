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
  valread = read(new_socket, buffer, 13);
  cout << "Received: " << buffer << endl;
  string bufferStr(buffer, 13);
  acknowledgeNumber = addSeqAckNumber(atoi(bufferStr.substr(3,6).data()), 1);
  if(atoi(bufferStr.substr(9,2).data()) != maxBuffer) {
    maxBuffer = min(maxBuffer, atoi(bufferStr.substr(9,2).data()));
  }
  if(atoi(bufferStr.substr(11,2).data()) != maxMessage) {
    maxMessage = min(maxMessage, atoi(bufferStr.substr(11,2).data()));
  }
  string sendStr = createSynAckPacket(sequenceNumber, acknowledgeNumber, maxBuffer, maxMessage, bufferStr);
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  delete [] buffer;
  char* newBuffer = new char[bufferVal];
  buffer = newBuffer;
  send(new_socket, sendStr.data(), 19, 0);
  cout << "Sent: " << sendStr << endl;
  valread = read(new_socket, buffer, 15);
  cout << "Received: "<< buffer << endl;
  bufferStr = bufferToString(buffer, 15);
  sequenceNumber = addSeqAckNumber(sequenceNumber, 1);
  if(atoi(bufferStr.substr(9,6).data()) != sequenceNumber) {
    acknowledgeNumber = addSeqAckNumber(atoi(bufferStr.substr(3,6).data()), 1);
    sendStr = createRstPacket(sequenceNumber, acknowledgeNumber);
    send(new_socket, sendStr.data(), sendStr.size(), 0);
    cout << "Sent: " << sendStr << endl;
    return -1;
  }
  return 0;
}
