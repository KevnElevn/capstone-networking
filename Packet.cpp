#include "Packet.h"
#include "Protocol.h"
#include <string>
#include <iostream>
#include <cstdlib>

extern const std::unordered_map<int, std::string> RSTERRORS;

Packet::Packet()
{
  size = 0;
  type = 9;
  sequence = 0;
  acknowledge = 0;
  field1 = 0;
  field2 = 0;
  data = "";
}

Packet::Packet(char packetType, int seq, int ack, int f1, int f2, std::string dat)
{
  setPacket(packetType, seq, ack, f1, f2, dat);
}

Packet::Packet(char* buffer)
{
  setPacket(buffer);
}

void Packet::setPacket(char packetType, int seq, int ack, int f1, int f2, std::string dat)
{
  type = packetType;
  sequence = seq;
  acknowledge = ack;
  field1 = f1;
  field2 = f2;
  data = dat;
  switch(type)
  {
    case SYN:
      size = PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH;
      break;

    case SAK:
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH;
      break;

    case REQ:
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+DATA_LENGTH_LENGTH+data.size();
      break;

    case RAK:
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+FILE_SIZE_LENGTH+DATA_LENGTH_LENGTH+data.size();
      break;

    case DAT:
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+DATA_LENGTH_LENGTH+data.size();
      break;

    case RST:
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+RST_ERROR_LENGTH;
      break;

    default:
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH);
  }
}

void Packet::setPacket(char* buffer)
{
  std::string str;
  for(int i=0; i<SEQ_ACK_LENGTH; i++)
    str += buffer[i];
  sequence = std::atoi(str.data());
  str.clear();
  if(type != SYN)
  {
    for(int i=SEQ_ACK_LENGTH; i<(2*SEQ_ACK_LENGTH); i++)
      str += buffer[i];
    acknowledge = std::atoi(str.data());
    str.clear();
  }
  switch(type)
  {
    case SYN:
    {
      for(int i=SEQ_ACK_LENGTH; i<SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH; i++)
        str += buffer[i];
      field1 = std::atoi(str.data());
      str.clear();
      for(int i=SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH; i<SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH; i++)
        str += buffer[i];
      field2 = std::atoi(str.data());
      str.clear();
      data.clear();
      size = PACKET_TYPE_LENGTH+SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH;
      break;
    }

    case SAK:
    {
      for(int i=(2*SEQ_ACK_LENGTH); i<(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH; i++)
        str += buffer[i];
      field1 = std::atoi(str.data());
      str.clear();
      for(int i=(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH; i<(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH; i++)
        str += buffer[i];
      field2 = std::atoi(str.data());
      str.clear();
      data.clear();
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH;
      break;
    }

    case REQ:
    {
      for(int i=(2*SEQ_ACK_LENGTH); i<(2*SEQ_ACK_LENGTH)+DATA_LENGTH_LENGTH; i++)
        str += buffer[i];
      field2 = std::atoi(str.data());
      str.clear();
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+DATA_LENGTH_LENGTH;
      break;
    }

    case RAK:
    {
      for(int i=(2*SEQ_ACK_LENGTH); i<(2*SEQ_ACK_LENGTH)+FILE_SIZE_LENGTH; i++)
        str += buffer[i];
      field1 = std::atoi(str.data());
      str.clear();
      for(int i=(2*SEQ_ACK_LENGTH)+FILE_SIZE_LENGTH; i<(2*SEQ_ACK_LENGTH)+FILE_SIZE_LENGTH+DATA_LENGTH_LENGTH; i++)
        str += buffer[i];
      field2 = std::atoi(str.data());
      str.clear();
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+FILE_SIZE_LENGTH+DATA_LENGTH_LENGTH;
      break;
    }

    case DAT:
    {
      for(int i=(2*SEQ_ACK_LENGTH); i<(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH; i++)
        str += buffer[i];
      field1 = std::atoi(str.data());
      str.clear();
      for(int i=(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH; i<(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+DATA_LENGTH_LENGTH; i++)
        str += buffer[i];
      field2 = std::atoi(str.data());
      str.clear();
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+DATA_LENGTH_LENGTH;
      break;
    }

    case RST:
    {
      for(int i=(2*SEQ_ACK_LENGTH); i<(2*SEQ_ACK_LENGTH)+RST_ERROR_LENGTH; i++)
        str += buffer[i];
      field1 = std::atoi(str.data());
      str.clear();
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH)+RST_ERROR_LENGTH;
      break;
    }

    default:
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH);
  }
}

void Packet::setType(char t)
{
  type = t;
  switch(type)
  {
    case SYN:
      size = SEQ_ACK_LENGTH+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH;
      break;

    case SAK:
      size = (2*SEQ_ACK_LENGTH)+BUFFER_SIZE_LENGTH+MESSAGE_SIZE_LENGTH;
      break;

    case REQ:
      size = (2*SEQ_ACK_LENGTH)+DATA_LENGTH_LENGTH;
      break;

    case RAK:
      size = (2*SEQ_ACK_LENGTH)+FILE_SIZE_LENGTH+DATA_LENGTH_LENGTH;
      break;

    case DAT:
      size = (2*SEQ_ACK_LENGTH)+CHUNK_ID_LENGTH+DATA_LENGTH_LENGTH;
      break;

    case RST:
      size = (2*SEQ_ACK_LENGTH)+RST_ERROR_LENGTH;
      break;

    default:
      size = PACKET_TYPE_LENGTH+(2*SEQ_ACK_LENGTH);
  }
}

void Packet::setData(char* buffer)
{
  data.clear();
  for(int i=0; i<field2; i++)
  {
    data += buffer[i];
  }
  size += field2;
}

void Packet::printPacket()
{
  switch(type)
  {
    case SYN:
    {
      std::cout << "SYN" << std::endl;
      std::cout << "Sequence Number: " << sequence << std::endl;
      std::cout << "Max buffer: " << field1 << std::endl;
      std::cout << "Max message: " << field2 << std::endl;
      break;
    }

    case SAK:
      {
        std::cout << "SAK" << std::endl;
        std::cout << "Sequence Number: " << sequence << std::endl;
        std::cout << "Acknowledge Number: " << acknowledge << std::endl;
        std::cout << "Max buffer: " << field1 << std::endl;
        std::cout << "Max message: " << field2 << std::endl;
        break;
    }

    case REQ:
    {
      std::cout << "REQ" <<std::endl;
      std::cout << "Sequence Number: " << sequence << std::endl;
      std::cout << "Acknowledge Number: " << acknowledge << std::endl;
      std::cout << "Message Length: " << field2 << std::endl;
      std::cout << "Filename: " << data << std::endl;
      break;
    }

    case RAK:
    {
      std::cout << "RAK" <<std::endl;
      std::cout << "Sequence Number: " << sequence << std::endl;
      std::cout << "Acknowledge Number: " << acknowledge << std::endl;
      std::cout << "File size: " << field1 << std::endl;
      std::cout << "Message Length: " << field2 << std::endl;
      std::cout << "Filename: " << data << std::endl;
      break;
    }

    case DAT:
    {
      std::cout << "DAT" <<std::endl;
      std::cout << "Sequence Number: " << sequence << std::endl;
      std::cout << "Acknowledge Number: " << acknowledge << std::endl;
      std::cout << "Chunk Number: " << field1 << std::endl;
      std::cout << "Message Length: " << field2 << std::endl;
      //std::cout << "Message: " << data << std::endl;
      break;
    }

    case RST:
    {
      std::cout << "RST" << std::endl;
      std::cout << "Sequence Number: " << sequence << std::endl;
      std::cout << "Acknowledge Number: " << acknowledge << std::endl;
      std::cout << "RST error code: " << field1 << std::endl;
      std::cout << "RST reason: " << RSTERRORS.at(field1) << std::endl;
      break;
    }

    default:
    {
        switch(type)
      {
        case ACK: std::cout << "ACK" << std::endl; break;
        case DON: std::cout << "DON" << std::endl; break;
        case FIN: std::cout << "FIN" << std::endl; break;
        case FAK: std::cout << "FAK" << std::endl; break;
      }
      std::cout << "Sequence Number: " << sequence << std::endl;
      std::cout << "Acknowledge Number: " << acknowledge << std::endl;
      break;
    }
  }
}

std::string Packet::toString() const
{
  std::string seq = std::to_string(sequence);
  seq.insert(seq.begin(), SEQ_ACK_LENGTH-seq.size(), '0');
  std::string ack;
  if(type != SYN)
  {
    ack = std::to_string(acknowledge);
    ack.insert(ack.begin(), SEQ_ACK_LENGTH-ack.size(), '0');
  }
  switch(type)
  {
    case SYN:
    {
      std::string bufferSizeStr = std::to_string(field1);
      std::string messageSizeStr = std::to_string(field2);
      bufferSizeStr.insert(bufferSizeStr.begin(), BUFFER_SIZE_LENGTH-bufferSizeStr.size(), '0');
      messageSizeStr.insert(messageSizeStr.begin(), MESSAGE_SIZE_LENGTH-messageSizeStr.size(), '0');

      return SYN + seq + bufferSizeStr + messageSizeStr;
      break;
    }

    case SAK:
    {
      std::string bufferSizeStr = std::to_string(field1);
      std::string messageSizeStr = std::to_string(field2);
      bufferSizeStr.insert(bufferSizeStr.begin(), BUFFER_SIZE_LENGTH-bufferSizeStr.size(), '0');
      messageSizeStr.insert(messageSizeStr.begin(), MESSAGE_SIZE_LENGTH-messageSizeStr.size(), '0');

      return SAK + seq + ack + bufferSizeStr + messageSizeStr;
      break;
    }

    case REQ:
    {
      std::string messageLengthStr = std::to_string(field1);
      messageLengthStr.insert(messageLengthStr.begin(), DATA_LENGTH_LENGTH-messageLengthStr.size(), '0');

      return REQ + seq + ack + messageLengthStr + data;
    }

    case RAK:
    {
      std::string fileSizeStr = std::to_string(field1);
      fileSizeStr.insert(fileSizeStr.begin(), FILE_SIZE_LENGTH-fileSizeStr.size(), '0');
      std::string messageLengthStr = std::to_string(field2);
      messageLengthStr.insert(messageLengthStr.begin(), DATA_LENGTH_LENGTH-messageLengthStr.size(), '0');

      return RAK + seq + ack + fileSizeStr + messageLengthStr + data;
    }

    case DAT:
    {
      std::string chunkId = std::to_string(field1);
      chunkId.insert(chunkId.begin(), FILE_SIZE_LENGTH-chunkId.size(), '0');
      std::string messageLengthStr = std::to_string(field2);
      messageLengthStr.insert(messageLengthStr.begin(), DATA_LENGTH_LENGTH-messageLengthStr.size(), '0');

      return DAT + seq + ack + chunkId + messageLengthStr + data;
    }

    case RST:
    {
      std::string rstCode = std::to_string(field1);
      rstCode.insert(rstCode.begin(), RST_ERROR_LENGTH-rstCode.size(), '0');

      return RST + seq + ack + rstCode;
    }

    default:
      return type + seq + ack;
  }

  return "";
}
