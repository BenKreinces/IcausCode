/** @file udp_service.h @brief LwIP raw-API UDP owner and bounded message bridge. */
#ifndef ICARUS_UDP_SERVICE_H
#define ICARUS_UDP_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "cmsis_os2.h"

#define ICARUS_UDP_COMMAND_PORT UINT16_C(5005)
#define ICARUS_UDP_TELEMETRY_PORT UINT16_C(5006)
#define ICARUS_UDP_MAX_DATAGRAM UINT16_C(532)

struct udp_pcb;

typedef enum
{
  ICARUS_UDP_INITIALIZING = 0,
  ICARUS_UDP_READY,
  ICARUS_UDP_FAULT
} icarus_udp_state_t;

typedef struct
{
  uint32_t source_ipv4;
  uint16_t source_port;
  uint16_t length;
  uint8_t data[ICARUS_UDP_MAX_DATAGRAM];
} icarus_udp_datagram_t;

typedef struct
{
  bool in_use;
  uint32_t destination_ipv4;
  uint16_t destination_port;
  uint16_t length;
  uint8_t data[ICARUS_UDP_MAX_DATAGRAM];
} icarus_udp_tx_slot_t;

typedef struct
{
  struct udp_pcb *command_pcb;
  struct udp_pcb *telemetry_pcb;
  osMessageQueueId_t receive_queue;
  osMutexId_t tx_mutex;
  /* Used only by the tcpip callback; the queue copies it before return. */
  icarus_udp_datagram_t rx_staging;
  icarus_udp_tx_slot_t tx_slots[2];
  volatile icarus_udp_state_t state;
  uint32_t rx_packets;
  uint32_t rx_drops;
  uint32_t tx_packets;
  uint32_t tx_drops;
  uint32_t socket_errors;
} icarus_udp_service_t;

bool IcarusUdpService_Init(icarus_udp_service_t *service);
bool IcarusUdpService_TryReceive(icarus_udp_service_t *service,
                                 icarus_udp_datagram_t *datagram);
bool IcarusUdpService_Send(icarus_udp_service_t *service, uint8_t slot_index,
                           uint32_t destination_ipv4, uint16_t destination_port,
                           const uint8_t *data, uint16_t length);

#endif /* ICARUS_UDP_SERVICE_H */
