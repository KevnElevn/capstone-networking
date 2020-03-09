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
    if(atoi(argv[1]) < 5) {
      //Buffer size < 32B
      cerr << "Buffer too small" << endl;
      return 1;
    }
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
  //Send SYN packet
  string sendStr = createSynPacket(sequenceNumber, maxBuffer, maxMessage);
  send(sock, sendStr.data(), PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH, 0);
  cout << "Sent: " << endl;
  printPacket(sendStr);
  //Read SYNACK packet
  valread = read(sock, buffer, PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH); //Read SYNACK packet
  string bufferStr(buffer, PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH);
  cout << "Received: " << endl;
  printPacket(bufferStr);
  //Check and process SYNACK packet
  if(!receiveSynAckPacket(sequenceNumber, acknowledgeNumber, maxBuffer, maxMessage, bufferStr)) {
    sendRstPacket(sock, sendStr, sequenceNumber, acknowledgeNumber);
    return -1;
  }
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  delete [] buffer;
  char* newBuffer = new char[bufferVal];
  buffer = newBuffer;
  sendStr = createCtrlPacket(ACK, sequenceNumber, acknowledgeNumber);
  send(sock, sendStr.data(), PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH), 0);
  cout << "Sent: " << endl;
  printPacket(sendStr);
  //Handshake complete
  string fullMessage;
  while(valread = read(sock, buffer, bufferVal)) {
    bufferStr = bufferToString(buffer, bufferVal);
    cout << "Received: " << endl;
    printPacket(bufferStr);
    if(!receiveDatPacket(bufferStr, fullMessage)) {
      cout << "Current message: " << fullMessage << endl;
    }
  }

  return 0;
}
