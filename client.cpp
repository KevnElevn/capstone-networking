#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstdlib>
#include <cmath>
#include "Protocol.h"
#define PORT 8080

using namespace std;

int main(int argc, char const *argv[]) {
  if(argc > 1) {
    if(atoi(argv[1]) > 25) {
      //Buffer size > ~33MB
      cerr << "Buffer too big" << endl;
      return 1;
    }
    if(atoi(argv[2]) > 30) {
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
  char* buffer = new char[32];
  int sequenceNumber = rand() % 1000000;
  int acknowledgeNumber = 0;
  int maxBuffer = 5;
  int maxMessage = 10;
  if(argc > 1) {
    maxBuffer = atoi(argv[1]);
    maxMessage = atoi(argv[2]);
  }
  string sendStr = createSynPacket(sequenceNumber, maxBuffer, maxMessage);
  send(sock, sendStr.data(), 13, 0);
  cout << "Sent: " << sendStr << endl;
  valread = read(sock, buffer, 19);
  cout << "Received: " << buffer << endl;
  string bufferStr(buffer, 19);
  acknowledgeNumber = addSeqAckNumber(atoi(bufferStr.substr(3,6).data()), 1);
  sequenceNumber = addSeqAckNumber(sequenceNumber, 1);
  if(atoi(bufferStr.substr(9,6).data()) == sequenceNumber) {
    if(atoi(bufferStr.substr(15,2).data()) != maxBuffer) {
      maxBuffer = min(maxBuffer, atoi(bufferStr.substr(15,2).data()));
    }
    if(atoi(bufferStr.substr(17,2).data()) != maxMessage) {
      maxMessage = min(maxMessage, atoi(bufferStr.substr(17,2).data()));
    }
    sendStr = createAckPacket(sequenceNumber, acknowledgeNumber);
  }
  else {
    sendStr = createRstPacket(sequenceNumber, acknowledgeNumber);
  }
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  delete [] buffer;
  char* newBuffer = new char[bufferVal];
  buffer = newBuffer;
  send(sock, sendStr.data(), 15, 0);
  cout << "Sent: " << sendStr << endl;
  //End connection if RST
  if(sendStr[0] == 'R')
    return -1;
  valread = read(sock, buffer, bufferVal);

  return 0;
}
