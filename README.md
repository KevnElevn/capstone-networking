# capstone-networking

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
  -Send RST packet to end connection

Unresponsive client or server:
1. Client sends SYN packet to server
2. Server responds with a SYNACK packet
3. Client does not reply with an ACK packet

How does a receiver of a packet check the expected sequence number?
What does it do when it receives an unexpected sequence number?
How will the sender know which packets to resend?
