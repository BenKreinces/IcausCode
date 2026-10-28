/** @file ethernet_link_monitor.c @brief Ethernet PHY diagnostic state logic. */
#include "ethernet_link_monitor.h"

#include <stdbool.h>
#include <stddef.h>

static ethernet_link_state_t EthernetLinkMonitor_StateFor(
    ksz8081_result_t result, const ksz8081_status_t *status)
{
  if (result != KSZ8081_RESULT_OK)
  {
    return ETHERNET_LINK_STATE_FAULT;
  }

  if ((status == NULL) || !status->link_up)
  {
    return ETHERNET_LINK_STATE_DOWN;
  }

  return status->auto_negotiation_complete ? ETHERNET_LINK_STATE_UP
                                            : ETHERNET_LINK_STATE_NEGOTIATING;
}

void EthernetLinkMonitor_Init(ethernet_link_monitor_t *monitor)
{
  if (monitor == NULL)
  {
    return;
  }

  *monitor = (ethernet_link_monitor_t){
      .state = ETHERNET_LINK_STATE_INITIALIZING,
      .last_result = KSZ8081_RESULT_ARGUMENT,
  };
}

bool EthernetLinkMonitor_Observe(ethernet_link_monitor_t *monitor,
                                 bool is_scan,
                                 ksz8081_result_t result,
                                 const ksz8081_status_t *status,
                                 uint32_t now_ms)
{
  ethernet_link_state_t next_state;
  ethernet_link_state_t previous_state;

  if (monitor == NULL)
  {
    return false;
  }

  previous_state = monitor->state;
  monitor->poll_count++;
  if (is_scan)
  {
    monitor->scan_count++;
  }

  monitor->last_result = result;
  next_state = EthernetLinkMonitor_StateFor(result, status);
  if ((result == KSZ8081_RESULT_OK) && (status != NULL))
  {
    monitor->phy_address = status->phy_address;
  }
  else if (result != KSZ8081_RESULT_OK)
  {
    monitor->fault_count++;
  }

  monitor->state = next_state;
  if (next_state == previous_state)
  {
    return false;
  }

  monitor->transition_count++;
  monitor->last_transition_ms = now_ms;
  if (next_state == ETHERNET_LINK_STATE_UP)
  {
    monitor->link_up_count++;
  }
  else if ((previous_state == ETHERNET_LINK_STATE_UP) &&
           (next_state != ETHERNET_LINK_STATE_UP))
  {
    monitor->link_down_count++;
  }

  return true;
}
