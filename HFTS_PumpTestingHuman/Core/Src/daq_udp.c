/**
 * @file daq_udp.c
 * @brief UDP DAQ: same IP/ports/MAC as Arduino. Define USE_LWIP and add LwIP middleware + LWIP/App with IP 192.168.1.2, MAC, bind 5000.
 */
#include "daq_udp.h"
#include <string.h>

#if defined(USE_LWIP)
#include "lwip.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#endif

static daq_udp_recv_cb_t s_recv_cb;

#if defined(USE_LWIP)
/* Destination: laptop 192.168.1.1, port 6000 (match Arduino) */
#define DAQ_PC_IP1   192
#define DAQ_PC_IP2   168
#define DAQ_PC_IP3   1
#define DAQ_PC_IP4   1
#define DAQ_PC_PORT  6000
#define DAQ_LOCAL_PORT 5000

static struct udp_pcb *s_udp_pcb;
static ip4_addr_t s_dst_ip;

#define DAQ_RECV_BUF_SIZE 64

static void daq_udp_lwip_recv_cb(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                                  const ip_addr_t *addr, u16_t port)
{
    (void)arg;
    (void)pcb;
    (void)port;
    (void)addr;
    if (p == NULL || s_recv_cb == NULL) {
        if (p != NULL) pbuf_free(p);
        return;
    }
    uint16_t len = p->tot_len;
    if (len > 0 && len <= DAQ_RECV_BUF_SIZE) {
        uint8_t buf[DAQ_RECV_BUF_SIZE];
        pbuf_copy_partial(p, buf, len, 0);
        uint8_t instruction = buf[0];
        const uint8_t *payload = (len > 1) ? &buf[1] : NULL;
        uint16_t payload_len = (len > 1) ? (uint16_t)(len - 1) : 0;
        s_recv_cb(instruction, payload, payload_len);
    }
    pbuf_free(p);
}
#endif

void DAQ_UDP_Init(void)
{
#if defined(USE_LWIP)
    s_udp_pcb = udp_new();
    if (s_udp_pcb != NULL) {
        err_t err = udp_bind(s_udp_pcb, IP_ADDR_ANY, DAQ_LOCAL_PORT);
        if (err == ERR_OK) {
            udp_recv(s_udp_pcb, daq_udp_lwip_recv_cb, NULL);
        }
    }
    IP4_ADDR(&s_dst_ip, DAQ_PC_IP1, DAQ_PC_IP2, DAQ_PC_IP3, DAQ_PC_IP4);
#endif
}

void DAQ_UDP_Process(void)
{
#if defined(USE_LWIP)
    MX_LWIP_Process();
#endif
}

void DAQ_UDP_SendPacket(const uint8_t *data, uint16_t len)
{
#if defined(USE_LWIP)
    if (s_udp_pcb == NULL || data == NULL || len == 0) return;
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) return;
    memcpy(p->payload, data, len);
    udp_sendto(s_udp_pcb, p, (const ip_addr_t *)&s_dst_ip, DAQ_PC_PORT);
    pbuf_free(p);
#else
    (void)data;
    (void)len;
#endif
}

void DAQ_UDP_SetRecvCallback(daq_udp_recv_cb_t cb)
{
    s_recv_cb = cb;
}
