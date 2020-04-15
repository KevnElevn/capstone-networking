# capstone-networking
Instructions to use:
1. Clone this repository onto a Linux machine
2. Have g++ and make installed. In the capstone-networking directory, run 'make server' and 'make client'
3. Run './server [max_buffer] [max_message]' to begin listening on port 7253.
   <max_buffer> affects 2^(max_buffer) and determines the maximum buffer allowed for communication. Default 10 -> 1KB
   <max_message> affects 2^(max_message) and determines the maximum file size allowed for file transfer. Default 30 -> 1GB
4. To read a file from another host running the server, run './client r <filename> <ip_address> [max_buffer] [max_message]'
  <max_buffer> affects 2^(max_buffer) and determines the maximum buffer allowed for communication. Default 10 -> 1KB
  <max_message> affects 2^(max_message) and determines the maximum file size allowed for file transfer. Default 30 -> 262MB
  If successful, the program will return a 0, and the requested file will be in the files directory.
5. To write a file to another host,  run './client r <filename> <ip_address> [max_buffer] [max_message]'
  If successful, the program will return a 0, and the given file will be written into the other host's files directory


Handshake:
1. Client sends SYN packet to server
  -SYN
  -SequenceNumber - randomly generated number from 0 - 999999
  -BufferSize - to be raised to power of 2
  -MaxMessageSize - to be raised to power of 2 (Message stored in RAM)
2. Server sends SYNACK packet to client
  -SAK
  -SequenceNumber - randomly generated number from 0 - 999999
  -AcknowledgeNumber - Received client's sequence number + 1
  -BufferSize - to be raised to power of 2
  -MaxMessageSize - to be raised to power of 2
3. Server sends ACK packet to server
  -ACK
  -SequenceNumber - In this case, previous sequence number + 1
  -AcknowledgeNumber - Received server's sequence number + 1
4. Server and client both set their max buffer and max message sizes to the agreed value (min(client's, server's))

Buffer Overflow:
1. Server or client tries to send a packet that's bigger than the agreed upon buffer size
2. Server or client's packet is truncated to agreed buffer size; entire packet is not sent
3. If the acknowledge number, sequence number, checksum, or other verification data isn't correctly formatted to Protocol
  -Send RST packet and connection

Unresponsive client or server:
1. Server and client have a timeout window
2. After listening for a response for a certain amount of time, the listener will send a RST packet and end the connection.

How does a receiver of a packet check the expected sequence number?
  When a packet is received, the size of the packet (or 1 if it's an initial handshake packet)is added to the receiver's acknowledge number. This is checked against the packet sender's sequence number.
What does it do when it receives an unexpected sequence number?
  The receiver will send a RST packet and end the connection.
  How will the sender know which packets to resend?
