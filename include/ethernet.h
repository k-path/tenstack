#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
#include <linux/if_ether.h>

#include "pktbuf.h"

#define ETH_DEBUG 0 // set to 1 to enable verbose Ethernet debugging

/* Ethertype values */
#define ETH_P_IP    0x0800 // Internet Protocol packet
#define ETH_P_ARP   0x0806 // Address Resolution Protocol packet
#define ETH_P_IPV6  0x86DD // IPv6

/* Ethernet header structure */
struct eth_header {
    uint8_t dest_mac[6]; // destination ether addr , 1 byte
    uint8_t src_mac[6]; // source ether addr
    uint16_t eth_type; // 2 bytes, packet type ID, indicates either length or type of the payload (e.g. IPv4, ARP), value is less than that it contains length of payload
} __attribute__((packed)); 

/* Transmit an Ethernet frame */
int ethernet_tx(struct pktbuf *pkt, const uint8_t *dst_mac, uint16_t ethertype);

/* Processing incoming Ethernet frames */
void ethernet_rx(struct pktbuf *pkt);

/* Initialize Ethernet layer */
void ethernet_init(void);

#endif 