/**
 * @file ethernet_link_monitor.h
 * @brief Bus-independent Ethernet PHY link-health state and counters.
 *
 * This module consumes successful or failed KSZ8081 diagnostic snapshots. It
 * neither accesses the MAC nor transmits packets, so it can be unit-tested on
 * a host and used before LwIP is introduced.
 */
#ifndef ETHERNET_LINK_MONITOR_H
#define ETHERNET_LINK_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

#include "ksz8081.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  ETHERNET_LINK_STATE_INITIALIZING = 0,
  ETHERNET_LINK_STATE_DOWN,
  ETHERNET_LINK_STATE_NEGOTIATING,
  ETHERNET_LINK_STATE_UP,
  ETHERNET_LINK_STATE_FAULT
} ethernet_link_state_t;

typedef struct
{
  ethernet_link_state_t state;
  ksz8081_result_t last_result;
  uint8_t phy_address;
  uint32_t poll_count;
  uint32_t scan_count;
  uint32_t fault_count;
  uint32_t link_up_count;
  uint32_t link_down_count;
  uint32_t transition_count;
  uint32_t last_transition_ms;
} ethernet_link_monitor_t;

/** Initialize an empty monitor before the first MDIO operation. */
void EthernetLinkMonitor_Init(ethernet_link_monitor_t *monitor);

/**
 * Record a bounded MDIO scan/status attempt.
 *
 * @param is_scan true for a PHY-address scan, false for a status refresh.
 * @param status Valid only when @p result is KSZ8081_RESULT_OK; otherwise NULL.
 * @return true only if the externally meaningful link state changed.
 */
bool EthernetLinkMonitor_Observe(ethernet_link_monitor_t *monitor,
                                 bool is_scan,
                                 ksz8081_result_t result,
                                 const ksz8081_status_t *status,
                                 uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* ETHERNET_LINK_MONITOR_H */
