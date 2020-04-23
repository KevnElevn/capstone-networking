#include <iostream>
//POSIX API
#include <unistd.h>
#include <sys/socket.h>
#include <cstdlib>
//Internet address family
#include <netinet/in.h>
#include <string>
#include <vector>
#include <ctime>
#include "Protocol.h"
#include "Packet.h"

using namespace std;

int main(int argc, char const *argv[])
{
  if(argc < 2)
    {
      cerr << "Enter a port to listen to" << endl;
      return 1;
    }
  if(argc > 2)
  {
    if(atoi(argv[2]) < 5)
    {
      //Buffer size < 32B
      cerr << "Buffer too small" << endl;
      return 1;
    }
    if(atoi(argv[2]) > 26)
    {
      //Buffer size > ~66MB
      cerr << "Buffer too big" << endl;
      return 1;
    }
    if(argc > 3)
    {
      if(atoi(argv[3]) > 31)
      {
        //Max message size > 2GB
        cerr << "Max message size too big" << endl;
        return 2;
      }
    }
  }

  int server_fd,  //socket descriptor
      new_socket; //used to accept connection request
  struct sockaddr_in address;
  int opt = 1;    //used for setsockopt() to manipulate options for the socket sockfd
  int addrlen = sizeof(address);
  short PORT = atoi(argv[1]);
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
  //Set recv timeout for socket
  // struct timeval tv;
  // tv.tv_sec = 5; //5 second timeout
  // tv.tv_usec = 0;
  // if(setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)& tv, sizeof tv))
  // {
  //   perror("setsockopt");
  //   exit(EXIT_FAILURE);
  // }

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
  while(true)
  {
    cout << "Awaiting connection..." << endl;
    if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    //Connected and listening
    cout << "Accepted socket number: " << new_socket << endl;
    srand(time(NULL)+1);
    Session session{rand()%1000000, 0, 10, 30, {}};
    session.buffer.reserve(32);
    if(argc > 2)
    {
      session.maxBuffer = atoi(argv[2]);
      if(argc > 3)
        session.maxMessage = atoi(argv[3]);
    }
    //Read SYN packet
    Packet packet;
    char packetType;
    int error;
    cout << "Session generated: " << session.sequenceNumber << ", " << session.acknowledgeNumber << endl;
    packetType = recvPacket(new_socket, session.buffer.data(), packet);
    if((error = handleSynPacket(new_socket, packet, session, packetType)) != 0)
    {
      cerr << RSTERRORS.at(error) << endl;
    }
    //Read ACK packet
    packetType = recvPacket(new_socket, session.buffer.data(), packet);
    if((error = handleAckPacket(new_socket, packet, session, packetType, 1)) != 0)
    {
      cerr << RSTERRORS.at(error) << endl;
    }
    //Inital handshake complete
    packetType = recvPacket(new_socket, session.buffer.data(), packet);
    while(true) //Assuming no timeout
    {
      session.acknowledgeNumber = addSeqAckNumber(session.acknowledgeNumber, packet.getSize());
      switch(packetType)
      {
        case REQ:
        {
          if((error = handleReqPacket(new_socket, packet, session)) != 0)
          {
            cerr << RSTERRORS.at(error) << endl;
            packetType = 'x'; //Stop recv-ing
          }
          else
            packetType = recvPacket(new_socket, session.buffer.data(), packet);
          break;
        }

        case FIN:
        {
          if((error = handleFinPacket(new_socket, packet, session)) != 0)
          {
            cerr << RSTERRORS.at(error) << endl;
          }
          close(new_socket);
          packetType = 'x'; //Stop recv-ing
          break;
        }

        case RST:
        {
          handleRstPacket(new_socket, packet, session);
          close(new_socket);
          packetType = 'x'; //Stop recv-ing
          break;
        }

        case 'e':
        {
          packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 97);
          sendPacket(new_socket, packet);
          close(new_socket);
          packetType = 'x';
        }

        default:
        {
          packet.setPacket(RST, session.sequenceNumber, session.acknowledgeNumber, 12);
          sendPacket(new_socket, packet);
          cerr << RSTERRORS.at(12) << endl;
          close(new_socket);
          packetType = 'x'; //Stop recv-ing
          break;
        }
      }
      if(packetType == 'x')
      break;
    }
  }
  return -1;
}
