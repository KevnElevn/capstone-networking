#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#define PORT 8080

using namespace std;

int main(int arg, char const *argv[]) {
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  char const *hello = "Hello from client";
  char buffer[1024] = {0};
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    cout << "Socket creation error \n";
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form (Network byte order)
  //sin_port and sin_addr must be in network byte order
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
  {
    cout << "Invalid address/ Address not supported \n";
    return -1;
  }

  if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    cout << "Connection failed \n";
  }
  send(sock, hello, strlen(hello), 0);
  cout << "Hello message sent\n";
  valread = read(sock, buffer, 1024);
  cout << buffer << endl;

  return 0;
}
