/**
 * @file daq_udp.h
 * @brief UDP DAQ: same IP/ports/MAC as Arduino. Define USE_LWIP and add LwIP middleware to enable.
 */
#ifndef DAQ_UDP_H
#define DAQ_UDP_H

/* Always enable LwIP/UDP path for this firmware. */
#ifndef USE_LWIP
#define USE_LWIP 1
#endif

#include <stdint.h>

/* Arduino/DAQ: ip 192.168.1.2, laptop 192.168.1.1:6000, local 5000, mac DE:AD:BE:EF:FE:ED */
void DAQ_UDP_Init(void);
void DAQ_UDP_Process(void);
void DAQ_UDP_SendPacket(const uint8_t *data, uint16_t len);

typedef void (*daq_udp_recv_cb_t)(uint8_t instruction_byte, const uint8_t *payload, uint16_t payload_len);
void DAQ_UDP_SetRecvCallback(daq_udp_recv_cb_t cb);

#endif /* DAQ_UDP_H */
