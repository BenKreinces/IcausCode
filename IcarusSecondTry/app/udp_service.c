#include "udp_service.h"

#include <string.h>

#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"

#define ICARUS_UDP_QUEUE_DEPTH UINT32_C(6)

static void UdpReceive(void *argument, struct udp_pcb *pcb, struct pbuf *pbuf,
                       const ip_addr_t *address, u16_t port)
{
  icarus_udp_service_t *service = argument;
  icarus_udp_datagram_t *datagram;
  (void)pcb;
  if ((service == NULL) || (pbuf == NULL) || (address == NULL) ||
      !IP_IS_V4(address) || (pbuf->tot_len > ICARUS_UDP_MAX_DATAGRAM))
  {
    if (service != NULL) ++service->rx_drops;
    if (pbuf != NULL) pbuf_free(pbuf);
    return;
  }
  datagram = &service->rx_staging;
  datagram->source_ipv4 = ip4_addr_get_u32(ip_2_ip4(address));
  datagram->source_port = port;
  datagram->length = pbuf->tot_len;
  if (pbuf_copy_partial(pbuf, datagram->data, datagram->length, 0U) != datagram->length ||
      (osMessageQueuePut(service->receive_queue, datagram, 0U, 0U) != osOK))
  {
    ++service->rx_drops;
  }
  else
  {
    ++service->rx_packets;
  }
  pbuf_free(pbuf);
}

static void UdpInitializeInCore(void *argument)
{
  icarus_udp_service_t *service = argument;
  err_t error;
  if (service == NULL) return;
  service->command_pcb = udp_new_ip_type(IPADDR_TYPE_V4);
  service->telemetry_pcb = udp_new_ip_type(IPADDR_TYPE_V4);
  if ((service->command_pcb == NULL) || (service->telemetry_pcb == NULL))
  {
    ++service->socket_errors;
    service->state = ICARUS_UDP_FAULT;
    return;
  }
  error = udp_bind(service->command_pcb, IP_ANY_TYPE, ICARUS_UDP_COMMAND_PORT);
  if (error == ERR_OK)
  {
    error = udp_bind(service->telemetry_pcb, IP_ANY_TYPE, ICARUS_UDP_TELEMETRY_PORT);
  }
  if (error != ERR_OK)
  {
    ++service->socket_errors;
    udp_remove(service->command_pcb);
    udp_remove(service->telemetry_pcb);
    service->command_pcb = NULL;
    service->telemetry_pcb = NULL;
    service->state = ICARUS_UDP_FAULT;
    return;
  }
  udp_recv(service->command_pcb, UdpReceive, service);
  service->state = ICARUS_UDP_READY;
}

typedef struct
{
  icarus_udp_service_t *service;
  uint8_t slot_index;
} icarus_udp_tx_context_t;

/* Two stable contexts avoid passing stack data across tcpip_callback(). */
static icarus_udp_tx_context_t tx_contexts[2];

static void UdpTransmitServiceInCore(void *argument)
{
  icarus_udp_tx_context_t *context = argument;
  icarus_udp_service_t *service;
  icarus_udp_tx_slot_t *slot;
  struct pbuf *pbuf;
  ip_addr_t destination;
  err_t error;

  if ((context == NULL) || (context->service == NULL) ||
      (context->slot_index >= 2U)) return;
  service = context->service;
  slot = &service->tx_slots[context->slot_index];
  if (osMutexAcquire(service->tx_mutex, 0U) != osOK) return;
  if (!slot->in_use)
  {
    (void)osMutexRelease(service->tx_mutex);
    return;
  }
  pbuf = pbuf_alloc(PBUF_TRANSPORT, slot->length, PBUF_RAM);
  if (pbuf == NULL)
  {
    ++service->tx_drops;
    slot->in_use = false;
    (void)osMutexRelease(service->tx_mutex);
    return;
  }
  error = pbuf_take(pbuf, slot->data, slot->length);
  ip_addr_set_ip4_u32(&destination, slot->destination_ipv4);
  if (error == ERR_OK)
  {
    error = udp_sendto(service->telemetry_pcb, pbuf, &destination,
                       slot->destination_port);
  }
  pbuf_free(pbuf);
  if (error == ERR_OK) ++service->tx_packets;
  else
  {
    ++service->tx_drops;
    ++service->socket_errors;
  }
  slot->in_use = false;
  (void)osMutexRelease(service->tx_mutex);
}

bool IcarusUdpService_Init(icarus_udp_service_t *service)
{
  static const osMutexAttr_t mutex_attributes = {.name = "UdpTx"};
  if (service == NULL) return false;
  memset(service, 0, sizeof(*service));
  service->receive_queue = osMessageQueueNew(ICARUS_UDP_QUEUE_DEPTH,
                                              sizeof(icarus_udp_datagram_t), NULL);
  service->tx_mutex = osMutexNew(&mutex_attributes);
  if ((service->receive_queue == NULL) || (service->tx_mutex == NULL))
  {
    service->state = ICARUS_UDP_FAULT;
    return false;
  }
  service->state = ICARUS_UDP_INITIALIZING;
  if (tcpip_callback(UdpInitializeInCore, service) != ERR_OK)
  {
    ++service->socket_errors;
    service->state = ICARUS_UDP_FAULT;
    return false;
  }
  return true;
}

bool IcarusUdpService_TryReceive(icarus_udp_service_t *service,
                                 icarus_udp_datagram_t *datagram)
{
  return (service != NULL) && (datagram != NULL) &&
         (osMessageQueueGet(service->receive_queue, datagram, NULL, 0U) == osOK);
}

bool IcarusUdpService_Send(icarus_udp_service_t *service, uint8_t slot_index,
                           uint32_t destination_ipv4, uint16_t destination_port,
                           const uint8_t *data, uint16_t length)
{
  icarus_udp_tx_slot_t *slot;
  if ((service == NULL) || (slot_index >= 2U) || (data == NULL) || (length == 0U) ||
      (length > ICARUS_UDP_MAX_DATAGRAM) || (service->state != ICARUS_UDP_READY))
  {
    return false;
  }
  if (osMutexAcquire(service->tx_mutex, 0U) != osOK) return false;
  slot = &service->tx_slots[slot_index];
  if (slot->in_use)
  {
    ++service->tx_drops;
    (void)osMutexRelease(service->tx_mutex);
    return false;
  }
  slot->destination_ipv4 = destination_ipv4;
  slot->destination_port = destination_port;
  slot->length = length;
  memcpy(slot->data, data, length);
  slot->in_use = true;
  tx_contexts[slot_index].service = service;
  tx_contexts[slot_index].slot_index = slot_index;
  (void)osMutexRelease(service->tx_mutex);
  if (tcpip_callback(UdpTransmitServiceInCore, &tx_contexts[slot_index]) != ERR_OK)
  {
    if (osMutexAcquire(service->tx_mutex, 0U) == osOK)
    {
      slot->in_use = false;
      ++service->tx_drops;
      (void)osMutexRelease(service->tx_mutex);
    }
    return false;
  }
  return true;
}
