#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ethernet_link_monitor.h"

static ksz8081_status_t MakeStatus(bool link_up, bool negotiation_complete)
{
  ksz8081_status_t status;

  memset(&status, 0, sizeof(status));
  status.phy_address = 7U;
  status.link_up = link_up;
  status.auto_negotiation_complete = negotiation_complete;
  return status;
}

int main(void)
{
  ethernet_link_monitor_t monitor;
  ksz8081_status_t status;

  EthernetLinkMonitor_Init(&monitor);
  assert(monitor.state == ETHERNET_LINK_STATE_INITIALIZING);

  assert(EthernetLinkMonitor_Observe(&monitor, true, KSZ8081_RESULT_NOT_FOUND,
                                     NULL, 10U));
  assert(monitor.state == ETHERNET_LINK_STATE_FAULT);
  assert(monitor.scan_count == 1U);
  assert(monitor.fault_count == 1U);

  status = MakeStatus(false, false);
  assert(EthernetLinkMonitor_Observe(&monitor, true, KSZ8081_RESULT_OK,
                                     &status, 20U));
  assert(monitor.state == ETHERNET_LINK_STATE_DOWN);
  assert(monitor.phy_address == 7U);

  status = MakeStatus(true, false);
  assert(EthernetLinkMonitor_Observe(&monitor, false, KSZ8081_RESULT_OK,
                                     &status, 30U));
  assert(monitor.state == ETHERNET_LINK_STATE_NEGOTIATING);

  status = MakeStatus(true, true);
  assert(EthernetLinkMonitor_Observe(&monitor, false, KSZ8081_RESULT_OK,
                                     &status, 40U));
  assert(monitor.state == ETHERNET_LINK_STATE_UP);
  assert(monitor.link_up_count == 1U);

  status = MakeStatus(false, false);
  assert(EthernetLinkMonitor_Observe(&monitor, false, KSZ8081_RESULT_OK,
                                     &status, 50U));
  assert(monitor.state == ETHERNET_LINK_STATE_DOWN);
  assert(monitor.link_down_count == 1U);
  assert(monitor.transition_count == 5U);
  assert(monitor.last_transition_ms == 50U);

  puts("Ethernet link monitor tests passed");
  return 0;
}
