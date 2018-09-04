# custom-protocol-parser
custom USART protocol parser

Input packet structure:  
| preamble |  cnt  |    type   |   length (data only) |   data and crc |  

   In order to optimize the computational resources the protocol parser is based on finite state machine for receiving packets. When the packet is assembled, specific flag sets immediately. Thus, other thread can stop the incoming packet and elaborate an appropriate response.

   For fault tolerance it is proposed to do the packet synchronization not only by analyzing the length field of the packet, but also using a hardware timer. When a preamble byte arrives, hardware timer starts counting from 0 up to period. The timer period elapsing interruption resets protocol FSM. 


