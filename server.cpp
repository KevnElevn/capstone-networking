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
#define PORT 8080

using namespace std;

string createSynAckPacket(int seq, int& ack, int bufferSize, int maxMessageSize, string& buff);
string createRstPacket(int seq, int ack);

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
  int bufferVal = static_cast<int>(pow(2,maxBuffer));
  int maxMessage = 10;
  int maxMessageVal = static_cast<int>(pow(2,maxMessage));
  if(argc > 1) {
    maxBuffer = atoi(argv[1]);
    maxMessage = atoi(argv[2]);
  }
  valread = read(new_socket, buffer, 14);
  cout << "Received: " << buffer << endl;
  if(buffer[13] != '\0') {
    string sendStr = createRstPacket(sequenceNumber, acknowledgeNumber);
    send(new_socket, sendStr.data(), sendStr.size(), 0);
    cout << "Sent: " << sendStr << endl;
    return -1;
  }
  string bufferStr;
  bufferStr = buffer;
  if(atoi(bufferStr.substr(9,2).data()) != maxBuffer) {
    maxBuffer = min(maxBuffer, atoi(bufferStr.substr(9,2).data()));
  }
  if(atoi(bufferStr.substr(11,2).data()) != maxMessage) {
    maxMessage = min(maxMessage, atoi(bufferStr.substr(11,2).data()));
  }
  string sendStr = createSynAckPacket(sequenceNumber, acknowledgeNumber, maxBuffer, maxMessage, bufferStr);
  delete [] buffer;
  char* newBuffer = new char[bufferVal];
  buffer = newBuffer;
  send(new_socket, sendStr.data(), sendStr.size(), 0);
  cout << "Sent: " << sendStr << endl;
  valread = read(new_socket, buffer, 16);
  cout << "Received: "<< buffer << endl;
  if(buffer[15] != '\0') {
    string sendStr = createRstPacket(sequenceNumber, acknowledgeNumber);
    send(new_socket, sendStr.data(), sendStr.size(), 0);
    cout << "Sent: " << sendStr << endl;
    return -1;
  }
  bufferStr = buffer;
  if(atoi(bufferStr.substr(9,6).data()) != ++sequenceNumber) {
    acknowledgeNumber = atoi(bufferStr.substr(3,6).data()) + 1;
    sendStr = createRstPacket(sequenceNumber, acknowledgeNumber);
    send(new_socket, sendStr.data(), sendStr.size(), 0);
    cout << "Sent: " << sendStr << endl;
    return -1;
  }
  return 0;
}

string createSynAckPacket(int seq, int& ack, int bufferSize, int maxMessageSize, string& buff) {
  string sequence = to_string(seq);
  string acknowledge = buff.substr(3,6);
  string bufferSizeStr = to_string(bufferSize);
  string messageSizeStr = to_string(maxMessageSize);
  ack = atoi(acknowledge.data()) + 1;
  if(ack > 999999)
    ack = 0;
  acknowledge = to_string(ack);
  sequence.insert(sequence.begin(), 6-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), 6-acknowledge.size(), '0');
  bufferSizeStr.insert(bufferSizeStr.begin(), 2-bufferSizeStr.size(), '0');
  messageSizeStr.insert(messageSizeStr.begin(), 2-messageSizeStr.size(), '0');
  return "SAK" + sequence + acknowledge + bufferSizeStr + messageSizeStr;
}

string createRstPacket(int seq, int ack) {
  string sequence = to_string(seq);
  string acknowledge = to_string(ack);
  sequence.insert(sequence.begin(), 6-sequence.size(), '0');
  acknowledge.insert(acknowledge.begin(), 6-acknowledge.size(), '0');
  return "RST" + sequence + acknowledge;
}
