#ifndef ETH_DEBUG_H
#define ETH_DEBUG_H

#include "stdint.h"
#include "stm32h7xx.h"

/* --- Live counters / flags --- */
extern volatile uint32_t g_link_up;
extern volatile uint32_t g_eth_started;
extern volatile uint32_t g_eth_irq;

extern volatile uint32_t g_rx_alloc_cb;
extern volatile uint32_t g_rx_link_cb;
extern volatile uint32_t g_rx_read_calls;
extern volatile uint32_t g_rx_pbuf_nonnull;
extern volatile uint32_t g_eth_input_calls;

extern volatile uint32_t g_tx_calls;
extern volatile uint32_t g_tx_last_tot_len;
extern volatile uint32_t g_tx_last_payload_addr;
extern volatile uint32_t g_tx_ok;
extern volatile uint32_t g_tx_err;

/* --- Optional: raw signal sampling --- */
extern volatile uint32_t g_phy_link_state;

/* --- HardFault capture --- */
extern volatile uint32_t g_HFSR;
extern volatile uint32_t g_CFSR;
extern volatile uint32_t g_CCR;
extern volatile uint32_t g_BFAR;
extern volatile uint32_t g_MMFAR;
extern volatile uint16_t g_UFSR;
extern volatile uint8_t g_BFSR;
extern volatile uint8_t g_MMFSR;

/* --- Interrupt state diagnostics --- */
extern volatile uint32_t g_primask;
extern volatile uint32_t g_basepri;
extern volatile uint32_t g_faultmask;

extern volatile uint32_t g_eth_ispr;
extern volatile uint32_t g_eth_iser;
extern volatile uint32_t g_eth_icer;
extern volatile uint32_t g_eth_itype; /* NVIC IPR byte (priority) */
extern volatile uint32_t g_eth_dmacier;
extern volatile uint32_t g_eth_dmacsr;
extern volatile uint32_t g_eth_macier;
extern volatile uint32_t g_eth_macisr;
extern volatile uint32_t g_eth_maciasr;

/* --- Descriptor state --- */
extern volatile uint32_t g_rx_desc0_status;
extern volatile uint32_t g_rx_desc0_buffer;
/* RX desc 0..3, RDES3: bit31=OWN, bit30=CTXT, bit29=FD, bit28=LD. Sample while pinging. */
extern volatile uint32_t g_rx_desc0_rdes3;
extern volatile uint32_t g_rx_desc1_rdes3;
extern volatile uint32_t g_rx_desc2_rdes3;
extern volatile uint32_t g_rx_desc3_rdes3;

/* --- PHY strap / mode --- */
extern volatile uint32_t g_phy_reg0;
extern volatile uint32_t g_phy_reg1;
extern volatile uint32_t g_phy_reg1F;

/* --- RMII pin sampling --- */
extern volatile uint32_t g_rmii_crs_dv;
extern volatile uint32_t g_rmii_rxd0;
extern volatile uint32_t g_rmii_rxd1;

/* --- RX descriptor full dump --- */
extern volatile uint32_t g_rx_desc0_d0;
extern volatile uint32_t g_rx_desc0_d1;
extern volatile uint32_t g_rx_desc0_d2;
extern volatile uint32_t g_rx_desc0_d3;

extern volatile uint32_t g_pa_idr;
extern volatile uint32_t g_pc_idr;

/* --- Last RX pbuf snapshot (inspect in Memory at g_rx_last_pbuf_payload, 64+ bytes) --- */
extern volatile uint32_t g_rx_last_pbuf_payload;
extern volatile uint32_t g_rx_last_pbuf_len;
extern volatile uint32_t g_rx_last_pbuf_tot_len;

/* --- ICMP ping (incremented in LwIP icmp_input when Echo Request received) --- */
extern volatile uint32_t g_icmp_echo_request_count;

/* --- IP layer (incremented in LwIP ip4_input) --- */
extern volatile uint32_t g_ip4_input_count;         /* every IP packet received */
extern volatile uint32_t g_ip4_input_accepted_count; /* packet accepted for this host (netif matched) */

/* --- UDP echo (incremented in lwip.c when UDP received on port 12345) --- */
extern volatile uint32_t g_udp_echo_recv_count;

/* --- UDP layer (incremented in LwIP udp.c) --- */
extern volatile uint32_t g_udp_input_count;   /* entered udp_input */
extern volatile uint32_t g_udp_recv_cb_count; /* about to call pcb->recv callback */

#endif
