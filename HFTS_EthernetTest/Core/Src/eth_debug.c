#include "eth_debug.h"

/* --- Live counters / flags --- */
volatile uint32_t g_link_up = 0;
volatile uint32_t g_eth_started = 0;
volatile uint32_t g_eth_irq = 0;

volatile uint32_t g_rx_alloc_cb = 0;
volatile uint32_t g_rx_link_cb = 0;
volatile uint32_t g_rx_read_calls = 0;
volatile uint32_t g_rx_pbuf_nonnull = 0;
volatile uint32_t g_eth_input_calls = 0;

volatile uint32_t g_tx_calls = 0;
volatile uint32_t g_tx_last_tot_len = 0;
volatile uint32_t g_tx_last_payload_addr = 0;
volatile uint32_t g_tx_ok = 0;
volatile uint32_t g_tx_err = 0;

volatile uint32_t g_phy_link_state = 0;

/* --- HardFault capture --- */
volatile uint32_t g_HFSR = 0;
volatile uint32_t g_CFSR = 0;
volatile uint32_t g_CCR = 0;
volatile uint32_t g_BFAR = 0;
volatile uint32_t g_MMFAR = 0;
volatile uint16_t g_UFSR = 0;
volatile uint8_t g_BFSR = 0;
volatile uint8_t g_MMFSR = 0;

/* --- Interrupt state diagnostics --- */
volatile uint32_t g_primask = 0;
volatile uint32_t g_basepri = 0;
volatile uint32_t g_faultmask = 0;

volatile uint32_t g_eth_ispr = 0;
volatile uint32_t g_eth_iser = 0;
volatile uint32_t g_eth_icer = 0;
volatile uint32_t g_eth_itype = 0;
volatile uint32_t g_eth_dmacier = 0;
volatile uint32_t g_eth_dmacsr = 0;
volatile uint32_t g_eth_macier = 0;
volatile uint32_t g_eth_macisr = 0;
volatile uint32_t g_eth_maciasr = 0;

/* --- Descriptor state --- */
volatile uint32_t g_rx_desc0_status = 0;
volatile uint32_t g_rx_desc0_buffer = 0;
volatile uint32_t g_rx_desc0_rdes3 = 0;
volatile uint32_t g_rx_desc1_rdes3 = 0;
volatile uint32_t g_rx_desc2_rdes3 = 0;
volatile uint32_t g_rx_desc3_rdes3 = 0;

/* --- PHY strap / mode --- */
volatile uint32_t g_phy_reg0 = 0;
volatile uint32_t g_phy_reg1 = 0;
volatile uint32_t g_phy_reg1F = 0;

/* --- RMII pin sampling --- */
volatile uint32_t g_rmii_crs_dv = 0;
volatile uint32_t g_rmii_rxd0   = 0;
volatile uint32_t g_rmii_rxd1   = 0;

/* --- RX descriptor full dump --- */
volatile uint32_t g_rx_desc0_d0 = 0;
volatile uint32_t g_rx_desc0_d1 = 0;
volatile uint32_t g_rx_desc0_d2 = 0;
volatile uint32_t g_rx_desc0_d3 = 0;

volatile uint32_t g_pa_idr = 0;
volatile uint32_t g_pc_idr = 0;

volatile uint32_t g_rx_last_pbuf_payload = 0;
volatile uint32_t g_rx_last_pbuf_len = 0;
volatile uint32_t g_rx_last_pbuf_tot_len = 0;

volatile uint32_t g_icmp_echo_request_count = 0;

volatile uint32_t g_ip4_input_count = 0;
volatile uint32_t g_ip4_input_accepted_count = 0;

volatile uint32_t g_udp_echo_recv_count = 0;

volatile uint32_t g_udp_input_count = 0;
volatile uint32_t g_udp_recv_cb_count = 0;
