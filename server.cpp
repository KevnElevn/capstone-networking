#include <iostream>
//POSIX API
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
//Internet address family
#include <netinet/in.h>
#include <cstring>
#include <string>
#define PORT 8080

using namespace std;

int main(int argc, char const *argv[])
{
    int server_fd,  //socket descriptor
        new_socket, //used to accept connection request
        valread;
    struct sockaddr_in address;
    int opt = 1;    //used for setsockopt() to manipulate options for the socket sockfd
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char const *hello = "Hello from server";

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
    cout << '1' << endl;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    /*
    */
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
    if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    valread = read(new_socket, buffer, 1024);
    cout << buffer << endl;
    send(new_socket, hello, strlen(hello), 0);
    cout << "Hello message sent \n";

    return 0;
}
