#include "protocol.h"    
#include "string.h"
#include "tim.h"
#include "usart.h"

static struct Packet rxPacket;

struct Packet fixRxPacket;
struct Packet txPacket;

struct ProtocolStatus prtclStatus;

static void recieve_preamble(const uint8_t rxByte);
static void recieve_cnt(const uint8_t rxByte);
static void recieve_type(const uint8_t rxByte);
static void recieve_lenght(const uint8_t rxByte);
static void recieve_data(const uint8_t rxByte);
static uint8_t calc_crc(const uint8_t *const in, size_t len);
static void reset_protocol(void);

static uint8_t calc_crc(const uint8_t * const  in, size_t len) {
    uint8_t crc = 0, inbyte, i, mix;
    const uint8_t* d = in;

    while (len--) {
        inbyte = *d++;
        for (i = 8; i; i--) {
            mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 0x01;
        }
    }
    return crc;
}

static void reset_protocol(void)
{
    prtclStatus.amountOfReceivedBytes = 0;
    prtclStatus.amountOfReceivedDataBytes = 0;
    memset(&rxPacket, 0x00, sizeof(rxPacket));
    prtclStatus.calcCrc = 0;
    prtclStatus.readCrc = 1;
}

void reset_timer(void)
{
// When the interrupt is triggered by the hardware timer, 
// the protocol will be reset and wait for the next packet.
    __HAL_TIM_SET_COUNTER(&htim1, 0);
}

void timer_interrupt(void) 
{
// When the interrupt is triggered by the hardware timer, 
// the protocol will be reset and wait for the next packet.
    prtclStatus.rxFlag = PKT_NOT_FOUND; // packet is receiving 
    reset_protocol();
}

static  void recieve_preamble(const uint8_t rxByte)
{
    if (PREAMBLE_BYTE_VALUE == rxByte)
    {
        rxPacket.preamble = rxByte;
        prtclStatus.amountOfReceivedBytes = 1;
        // It is supposed that in following 100ms a packet will be received.
        reset_timer();
        prtclStatus.rxFlag = PKT_IN_PROGRESS; // packet is receiving 
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
    } else {
        prtclStatus.amountOfReceivedBytes = 0;
    }
}

static void recieve_cnt(const uint8_t rxByte)
{
    rxPacket.cnt = rxByte;
    prtclStatus.amountOfReceivedBytes = 2;
    reset_timer();
}

static void recieve_type(const uint8_t rxByte)
{
    // Check the allowable types
    // if all values are  allowable types then:
    rxPacket.type = rxByte;
    prtclStatus.amountOfReceivedBytes = 3;
    reset_timer();
}

static void recieve_lenght(const uint8_t rxByte)
{
    rxPacket.length = rxByte;
    prtclStatus.amountOfReceivedBytes = 4;
    prtclStatus.amountOfReceivedDataBytes = 0;
    reset_timer();
}

static void recieve_data(const uint8_t rxByte)
{
    reset_timer();
    if (prtclStatus.amountOfReceivedDataBytes <= (rxPacket.length))
    {
        rxPacket.data[prtclStatus.amountOfReceivedDataBytes] = rxByte;
        prtclStatus.amountOfReceivedBytes++;
        prtclStatus.amountOfReceivedDataBytes++;
    }
    if (prtclStatus.amountOfReceivedDataBytes == (rxPacket.length+1))
    {
        prtclStatus.readCrc = rxByte;
        prtclStatus.calcCrc = calc_crc( (uint8_t *) &rxPacket, prtclStatus.amountOfReceivedBytes-1);
        if ( prtclStatus.readCrc == prtclStatus.calcCrc ) {
            //Perfectly! the package is accepted correctly.
            memcpy(&fixRxPacket, &rxPacket, prtclStatus.amountOfReceivedBytes); 
            prtclStatus.rxFlag = PKT_RECEIVED; // Show that packet has been received
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
            reset_protocol();
        } else {
            reset_protocol();
        }
    }
    if ( (rxPacket.length+1) < prtclStatus.amountOfReceivedDataBytes) {
        reset_protocol();
    }
}

// Function parse byte must be called in UART RX byte interruption.
void parse_byte(const uint8_t rxByte)
{
    switch (prtclStatus.amountOfReceivedBytes)
    {
        case 0: recieve_preamble(rxByte); break;
        case 1: recieve_cnt(rxByte); break;
        case 2: recieve_type(rxByte); break;
        case 3: recieve_lenght(rxByte); break;
        default: 
            recieve_data(rxByte); break;
    }
}

void proc_pkt(const struct Packet * const inPacket)
{
    memcpy(&txPacket, inPacket, HEADER_LEN + inPacket->length+1);
    txPacket.type |= (1<<7);
    txPacket.data[(inPacket->length)] = calc_crc( (uint8_t *) &txPacket,
                                                     HEADER_LEN+(inPacket->length));
    memcpy(uart1TxBuffer, &txPacket, HEADER_LEN+(inPacket->length)+1);
    g_flag_uart1Tx = 0;
    HAL_UART_Transmit_IT(&huart1, uart1TxBuffer, inPacket->length + HEADER_LEN+1);
    //wait for the end of tx interruption
    while(!g_flag_uart1Tx )
    {
    }
    
}
