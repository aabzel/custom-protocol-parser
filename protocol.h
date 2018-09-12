#ifndef __protocol_H
#define __protocol_H

#include "stm32f1xx_hal.h"

#define PREAMBLE_BYTE_VALUE   (0xFF)

enum pkt_fields
{
    PREAMBLE_BYTE = 0, 
    CNT_BYTE = 1,
    TYPE_BYTE = 2,
    LENGTH_BYTE = 3,
    HEADER_LEN = 4
};

enum input_pkt_status 
{
  PKT_NOT_FOUND = 0,
  PKT_IN_PROGRESS = 1,
  PKT_RECEIVED = 2,
  PKT_PROC_RESP = 3
}; 

struct Packet
{
    uint8_t preamble;       // preamble = 0xFF
    uint8_t cnt;            // packet counter
    uint8_t type;           // type of package
    uint8_t length;         // only data field length (without CRC byte)
    uint8_t data[255+1];    // data array   CRC8
}; // 260 byte

struct ProtocolStatus
{
  int32_t amountOfReceivedBytes;
  int32_t amountOfReceivedDataBytes;
  uint8_t readCrc;
  uint8_t calcCrc;
  uint8_t rxFlag;  
}; // 12 byte

extern struct ProtocolStatus prtclStatus;

extern struct Packet fixRxPacket;
extern struct Packet txPacket;

void suspend_timer(void);
void resume_timer(void);
void reset_timer(void);
void timer_interrupt(void);
void parse_byte(const uint8_t rxByte);
void proc_pkt(const struct Packet * const inPacket);

#endif /*__protocol_H */
