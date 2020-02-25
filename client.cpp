#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cstdlib>
#include <cmath>
#define PORT 8080

using namespace std;

string createSynPacket(int seq, int bufferSize, int maxMessageSize);
//bufferSize and maxMessageSize is exponent to raise 2 by.
string createAckPacket(int seq, int ack);
string createRstPacket(int seq, int ack);

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
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessage = 10;
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  if(argc > 1) {
    maxBuffer = atoi(argv[1]);
    maxMessage = atoi(argv[2]);
  }
  string sendStr = createSynPacket(sequenceNumber, maxBuffer, maxMessage);
  send(sock, sendStr.data(), sendStr.size(), 0);
  cout << "Sent: " << sendStr << endl;
  valread = read(sock, buffer, 20);
  cout << "Received: " << buffer << endl;
  if(buffer[19] != '\0') {
    string sendStr = createRstPacket(sequenceNumber, acknowledgeNumber);
    send(sock, sendStr.data(), sendStr.size(), 0);
    cout << "Sent: " << sendStr << endl;
    return -1;
  }
  string bufferStr;
  bufferStr = buffer;
  acknowledgeNumber = atoi(bufferStr.substr(3,6).data()) + 1;
  if(atoi(bufferStr.substr(9,6).data()) == ++sequenceNumber) {
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
  if(sequenceNumber == 1000000)
    sequenceNumber = 0;
  delete [] buffer;
  char* newBuffer = new char[bufferVal];
  buffer = newBuffer;
  send(sock, sendStr.data(), sendStr.size(), 0);
  cout << "Sent: " << sendStr << endl;
  //End connection if RST
  if(sendStr[0] == 'R')
    return -1;
  valread = read(sock, buffer, bufferVal);

  return 0;
}

string createSynPacket(int seq, int maxBufferSize, int maxMessageSize) {
  string sequence = to_string(seq);
  string bufferSizeStr = to_string(maxBufferSize);
  string messageSizeStr = to_string(maxMessageSize);
  sequence.insert(sequence.begin(), 6-sequence.size(), '0');
  bufferSizeStr.insert(bufferSizeStr.begin(), 2-bufferSizeStr.size(), '0');
  messageSizeStr.insert(messageSizeStr.begin(), 2-messageSizeStr.size(), '0');
  return "SYN" + sequence + bufferSizeStr + messageSizeStr;
}

string createAckPacket(int seq, int ack) {
  string sequence = to_string(seq);
  string acknowledge = to_string(ack);
  sequence.insert(sequence.begin(), 6-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), 6-acknowledge.size(), '0');
  return "ACK" + sequence + acknowledge;
}

string createRstPacket(int seq, int ack) {
  string sequence = to_string(seq);
  string acknowledge = to_string(ack);
  sequence.insert(sequence.begin(), 6-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), 6-acknowledge.size(), '0');
  return "RST" + sequence + acknowledge;
}
