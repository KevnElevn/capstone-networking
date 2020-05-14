# capstone-networking
Instructions to use:
1. Clone this repository onto a Linux machine

2. Have **g++** and **make** installed. In the capstone-networking directory, run `make server` and `make client`

3. Create a directory for each node. Within each directory, create a directory called 'files'

4. Copy and paste the server and client programs into each node's directory

**This project should be used with [this repo](https://github.com/KevnElevn/capstone-filesystem)**

Standalone instructions:
5. Run `./server <port> [max_buffer] [max_message]` to begin listening on given port.

   <max_buffer> affects 2^(max_buffer) and determines the maximum buffer allowed for communication. Default 10 -> 1KB

   <max_message> affects 2^(max_message) and determines the maximum file size allowed for file transfer. Default 30 -> 1GB

6. To read a file from another host running the server, run `./client r <filename> <port> [max_buffer] [max_message]`

  <max_buffer> affects 2^(max_buffer) and determines the maximum buffer allowed for communication. Default 10 -> 1KB

  <max_message> affects 2^(max_message) and determines the maximum file size allowed for file transfer. Default 30 -> 262MB

  If successful, the program will return a 0, and the requested file will be in the node directory.

7. To write a file to another host,  run `./client r <filename> <port> [max_buffer] [max_message]`

  If successful, the program will return a 0, and the given file will be written into the other host's files directory

8. To write a string to another host's 'updates.txt' file, run `./client u "theString" <port>`
