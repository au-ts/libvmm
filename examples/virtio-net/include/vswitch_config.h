#pragma once
#include <microkit.h>
#include <sddf/network/queue.h>
#include <sddf/network/vswitch.h>
#include <ethernet_config.h>

/* Number of ports we can recieve from and forward to */
#define VSWITCH_PORT_COUNT 3
/* Total number of destination MAC addresses that can be used for forwarding. 
 * Note that many MAC addresses may refer to a single port when the port is a
 * virtualiser or vswitch */
#define VSWITCH_LOOKUP_SIZE 256

#ifdef VSWITCH_IMPL
#define NET_Q(name) net_queue_t *name
#define NET_D(name) char *name##_data
#else
#define NET_Q(name)
#define NET_D(name)
#endif

#define NET_PORT(name) NET_Q(name##_tx_free); \
                       NET_Q(name##_tx_active); \
                       NET_Q(name##_rx_free); \
                       NET_Q(name##_rx_active); \
                       NET_D(name##_tx); \
                       NET_D(name##_rx)

NET_PORT(virt);
NET_PORT(cli0);
NET_PORT(cli1);

#define VSWITCH_VIRT_TX_CH 0
#define VSWITCH_VIRT_RX_CH 1
#define VSWITCH_CLI0_TX_CH 2
#define VSWITCH_CLI0_RX_CH 3
#define VSWITCH_CLI1_TX_CH 4
#define VSWITCH_CLI1_RX_CH 5

#ifdef VSWITCH_IMPL
static inline void config_init_channel(vswitch_channel_t *channel, net_queue_t *free, net_queue_t *active,
                                       char *data_region, size_t size, microkit_channel ch)
{
    net_queue_init(&channel->q, free, active, size);
    channel->data_region = data_region;
    channel->ch = ch;
}

static inline void net_vswitch_init_port(int index, vswitch_port_t *port)
{
    switch (index) {
    case 0:
        // For virtualisers, incoming is RX, outgoing is TX
        config_init_channel(&port->incoming, virt_rx_free, virt_rx_active, virt_rx_data, NET_RX_QUEUE_SIZE_DRIV, VSWITCH_VIRT_RX_CH);
        config_init_channel(&port->outgoing, virt_tx_free, virt_tx_active, virt_tx_data, NET_RX_QUEUE_SIZE_DRIV, VSWITCH_VIRT_TX_CH);
        port->allow_list = 0b111;
        break;
    case 1:
        // For clients, incoming is TX, outgoing is RX
        config_init_channel(&port->incoming, cli0_tx_free, cli0_tx_active, cli0_tx_data, NET_TX_QUEUE_SIZE_DRIV, VSWITCH_CLI0_TX_CH);
        config_init_channel(&port->outgoing, cli0_rx_free, cli0_rx_active, cli0_rx_data, NET_RX_QUEUE_SIZE_DRIV, VSWITCH_CLI0_RX_CH);
        port->allow_list = 0b111;
        break;
    case 2:
        config_init_channel(&port->incoming, cli1_tx_free, cli1_tx_active, cli1_tx_data, NET_TX_QUEUE_SIZE_DRIV, VSWITCH_CLI1_TX_CH);
        config_init_channel(&port->outgoing, cli1_rx_free, cli1_rx_active, cli1_rx_data, NET_RX_QUEUE_SIZE_DRIV, VSWITCH_CLI1_RX_CH);
        port->allow_list = 0b111;
        break;
    default:
        assert(0);
    }
}
#endif
