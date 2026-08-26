/*
 * udp_helper.c
 *
 *  Created on: Mar 14, 2026
 *      Author: alexi
 */


#include "main.h"

#include "udp_helper.h"
/*
void udpClient_connect(void)
{
    err_t err;

     1. Create a new UDP control block
    upcb = udp_new();

     Bind the block to module's IP and port
    ip_addr_t myIPaddr;
    IP_ADDR4(&myIPaddr, 192, 168, 0, 123);
    udp_bind(upcb, &myIPaddr, 6000);

     configure destination IP address and port
    ip_addr_t DestIPaddr;
    IP_ADDR4(&DestIPaddr, 192, 168, 1, 1);
    err = udp_connect(upcb, &DestIPaddr, 5000);

    if (err == ERR_OK)
    {
         2. Send message to server
        udpClient_send();

         3. Set a receive callback for the upcb
        udp_recv(upcb, udp_receive_callback, NULL);
    }
}

void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                           const ip_addr_t *addr, u16_t port)
{
     Copy the data from the pbuf
    strncpy(buffer, (char *)p->payload, p->len);

     increment message count
    counter++;

     Free receive pbuf
    pbuf_free(p);
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    udpClient_send();
}

static void udpClient_send(void)
{
    struct pbuf *txBuf;
    char data[100];

    int len = sprintf(data, "sending UDP client message %d", counter);

     allocate pbuf from pool
    txBuf = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);

    if (txBuf != NULL)
    {
         copy data to pbuf
        pbuf_take(txBuf, data, len);

         send udp data
        udp_send(upcb, txBuf);

         free pbuf
        pbuf_free(txBuf);
    }
}*/

