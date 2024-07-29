#pragma once

#include <microkit.h>
#include <sddf/util/string.h>
#include <sddf/network/queue.h>
#include <sddf/util/util.h>

#define NUM_NETWORK_CLIENTS 2

// We may need to change how this system works since now we have vswitch
// =======================================================================
// Now:
//  Client0 <----> copy0 <-----> virt_rx <-------> | ------ |
//  Client1 <----> copy1 <------/                  | Driver |
// (Client0) <-----------------> virt_tx <-------> | ------ |
// (Client1) <------------------/
//
// VSwitch:
//  Client0 <----> | ------- | <----> virt_rx <----> | ------ |
//                 | VSwitch |                       | Driver |
//  Client1 <----> | ------- | <----> virt_tx <----> | ------ |
//                  ^(move copy to vswitch?)

#define NET_CLI0_NAME "CLIENT_VMM-1"
#define NET_CLI1_NAME "CLIENT_VMM-2"
// #define NET_VSWITCH_NAME "vswitch"
#define NET_COPY0_NAME "copy1"
#define NET_COPY1_NAME "copy2"
#define NET_VIRT_RX_NAME "net_virt_rx"
#define NET_VIRT_TX_NAME "net_virt_tx"
#define NET_DRIVER_NAME "eth"
#define NET_TIMER_NAME "timer"

#define NET_DATA_REGION_SIZE                    0x200000
#define NET_HW_REGION_SIZE                      0x10000

#if defined(CONFIG_PLAT_IMX8MM_EVK)
#define MAC_ADDR_CLI0                       0x525401000001
#define MAC_ADDR_CLI1                       0x525401000002
#elif defined(CONFIG_PLAT_ODROIDC4)
#define MAC_ADDR_CLI0                       0x525401000003
#define MAC_ADDR_CLI1                       0x525401000004
#elif defined(CONFIG_PLAT_MAAXBOARD)
#define MAC_ADDR_CLI0                       0x525401000005
#define MAC_ADDR_CLI1                       0x525401000006
#elif defined(CONFIG_PLAT_QEMU_ARM_VIRT)
#define MAC_ADDR_CLI0                       0x525401000007
#define MAC_ADDR_CLI1                       0x525401000008
#else
#error "Must define MAC addresses for clients in ethernet config"
#endif

#define NET_TX_QUEUE_SIZE_CLI0                   512
#define NET_TX_QUEUE_SIZE_CLI1                   512
#define NET_TX_QUEUE_SIZE_DRIV                   (NET_TX_QUEUE_SIZE_CLI0 + NET_TX_QUEUE_SIZE_CLI1)

#define NET_TX_DATA_REGION_SIZE_CLI0            NET_DATA_REGION_SIZE
#define NET_TX_DATA_REGION_SIZE_CLI1            NET_DATA_REGION_SIZE

_Static_assert(NET_TX_DATA_REGION_SIZE_CLI0 >= NET_TX_QUEUE_SIZE_CLI0 *NET_BUFFER_SIZE,
               "Client0 TX data region size must fit Client0 TX buffers");
_Static_assert(NET_TX_DATA_REGION_SIZE_CLI1 >= NET_TX_QUEUE_SIZE_CLI1 *NET_BUFFER_SIZE,
               "Client1 TX data region size must fit Client1 TX buffers");

#define NET_RX_QUEUE_SIZE_DRIV                   512
#define NET_RX_QUEUE_SIZE_CLI0                   512
#define NET_RX_QUEUE_SIZE_CLI1                   512
#define NET_MAX_CLIENT_QUEUE_SIZE                MAX(NET_RX_QUEUE_SIZE_CLI0, NET_RX_QUEUE_SIZE_CLI1)
#define NET_RX_QUEUE_SIZE_COPY0                  NET_RX_QUEUE_SIZE_DRIV
#define NET_RX_QUEUE_SIZE_COPY1                  NET_RX_QUEUE_SIZE_DRIV

#define NET_RX_DATA_REGION_SIZE_DRIV            NET_DATA_REGION_SIZE
#define NET_RX_DATA_REGION_SIZE_CLI0            NET_DATA_REGION_SIZE
#define NET_RX_DATA_REGION_SIZE_CLI1            NET_DATA_REGION_SIZE

_Static_assert(NET_RX_DATA_REGION_SIZE_DRIV >= NET_RX_QUEUE_SIZE_DRIV *NET_BUFFER_SIZE,
               "Driver RX data region size must fit Driver RX buffers");
_Static_assert(NET_RX_DATA_REGION_SIZE_CLI0 >= NET_RX_QUEUE_SIZE_CLI0 *NET_BUFFER_SIZE,
               "Client0 RX data region size must fit Client0 RX buffers");
_Static_assert(NET_RX_DATA_REGION_SIZE_CLI1 >= NET_RX_QUEUE_SIZE_CLI1 *NET_BUFFER_SIZE,
               "Client1 RX data region size must fit Client1 RX buffers");

#define NET_MAX_QUEUE_SIZE MAX(NET_TX_QUEUE_SIZE_DRIV, MAX(NET_RX_QUEUE_SIZE_DRIV, MAX(NET_RX_QUEUE_SIZE_CLI0, NET_RX_QUEUE_SIZE_CLI1)))
_Static_assert(NET_TX_QUEUE_SIZE_DRIV >= NET_TX_QUEUE_SIZE_CLI0 + NET_TX_QUEUE_SIZE_CLI1,
               "Driver TX queue must have capacity to fit all of client's TX buffers.");
_Static_assert(NET_RX_QUEUE_SIZE_COPY0 >= NET_RX_QUEUE_SIZE_DRIV,
               "Copy0 queues must have capacity to fit all RX buffers.");
_Static_assert(NET_RX_QUEUE_SIZE_COPY1 >= NET_RX_QUEUE_SIZE_DRIV,
               "Copy1 queues must have capacity to fit all RX buffers.");
_Static_assert(sizeof(net_queue_t) + NET_MAX_QUEUE_SIZE *sizeof(net_buff_desc_t) <= NET_DATA_REGION_SIZE,
               "net_queue_t must fit into a single data region.");

static void __net_set_mac_addr(uint8_t *mac, uint64_t val)
{
    mac[0] = val >> 40 & 0xff;
    mac[1] = val >> 32 & 0xff;
    mac[2] = val >> 24 & 0xff;
    mac[3] = val >> 16 & 0xff;
    mac[4] = val >> 8 & 0xff;
    mac[5] = val & 0xff;
}

static inline void net_cli_mac_addr_init_sys(char *pd_name, uint8_t *macs)
{
    if (!sddf_strcmp(pd_name, NET_CLI0_NAME)) {
        __net_set_mac_addr(macs, MAC_ADDR_CLI0);
    } else if (!sddf_strcmp(pd_name, NET_CLI1_NAME)) {
        __net_set_mac_addr(macs, MAC_ADDR_CLI1);
    }
}

static inline void net_virt_mac_addr_init_sys(char *pd_name, uint8_t *macs)
{
    if (!sddf_strcmp(pd_name, NET_VIRT_RX_NAME)) {
        __net_set_mac_addr(macs, MAC_ADDR_CLI0);
        __net_set_mac_addr(&macs[ETH_HWADDR_LEN], MAC_ADDR_CLI1);
    }
}

static inline void net_cli_queue_init_sys(char *pd_name, net_queue_handle_t *rx_queue, net_queue_t *rx_free,
                                          net_queue_t *rx_active, net_queue_handle_t *tx_queue, net_queue_t *tx_free,
                                          net_queue_t *tx_active)
{
    if (!sddf_strcmp(pd_name, NET_CLI0_NAME)) {
        net_queue_init(rx_queue, rx_free, rx_active, NET_RX_QUEUE_SIZE_CLI0);
        net_queue_init(tx_queue, tx_free, tx_active, NET_TX_QUEUE_SIZE_CLI0);
    } else if (!sddf_strcmp(pd_name, NET_CLI1_NAME)) {
        net_queue_init(rx_queue, rx_free, rx_active, NET_RX_QUEUE_SIZE_CLI1);
        net_queue_init(tx_queue, tx_free, tx_active, NET_TX_QUEUE_SIZE_CLI1);
    }
}

static inline void net_copy_queue_init_sys(char *pd_name, net_queue_handle_t *cli_queue, net_queue_t *cli_free,
                                           net_queue_t *cli_active, net_queue_handle_t *virt_queue, net_queue_t *virt_free,
                                           net_queue_t *virt_active)
{
    if (!sddf_strcmp(pd_name, NET_COPY0_NAME)) {
        net_queue_init(cli_queue, cli_free, cli_active, NET_RX_QUEUE_SIZE_CLI0);
        net_queue_init(virt_queue, virt_free, virt_active, NET_RX_QUEUE_SIZE_COPY0);
    } else if (!sddf_strcmp(pd_name, NET_COPY1_NAME)) {
        net_queue_init(cli_queue, cli_free, cli_active, NET_RX_QUEUE_SIZE_CLI1);
        net_queue_init(virt_queue, virt_free, virt_active, NET_RX_QUEUE_SIZE_COPY1);
    }
}

static inline void net_virt_queue_init_sys(char *pd_name, net_queue_handle_t *cli_queue, net_queue_t *cli_free,
                                           net_queue_t *cli_active)
{
    if (!sddf_strcmp(pd_name, NET_VIRT_RX_NAME)) {
        net_queue_init(cli_queue, cli_free, cli_active, NET_RX_QUEUE_SIZE_COPY0);
        net_queue_init(&cli_queue[1], (net_queue_t *)((uintptr_t)cli_free + 2 * NET_DATA_REGION_SIZE),
                       (net_queue_t *)((uintptr_t)cli_active + 2 * NET_DATA_REGION_SIZE), NET_RX_QUEUE_SIZE_COPY1);
    } else if (!sddf_strcmp(pd_name, NET_VIRT_TX_NAME)) {
        net_queue_init(cli_queue, cli_free, cli_active, NET_TX_QUEUE_SIZE_CLI0);
        net_queue_init(&cli_queue[1], (net_queue_t *)((uintptr_t)cli_free + 2 * NET_DATA_REGION_SIZE),
                       (net_queue_t *)((uintptr_t)cli_active + 2 * NET_DATA_REGION_SIZE), NET_TX_QUEUE_SIZE_CLI1);
    }
}

static inline void net_mem_region_init_sys(char *pd_name, uintptr_t *mem_regions, uintptr_t start_region)
{
    if (!sddf_strcmp(pd_name, NET_VIRT_TX_NAME)) {
        mem_regions[0] = start_region;
        mem_regions[1] = start_region + NET_DATA_REGION_SIZE;
    }
}
