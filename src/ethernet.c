#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "ethernet.h"
#include "arp.h"
#include "netdev.h"

/* debug output macro */
#define eth_dbg(fmt, ...) \
    printf("ETH: " fmt "\n", ##__VA_ARGS__)

/* print Ethernet address (MAC) */
static void print_eth_addr(const char *name, const uint8_t *addr) {
    printf("%s: %02x:%02x:%02x:%02x:%02x:%02x\n", name, 
        addr[0], addr[1], addr[2], addr[3], addr[4],addr[5]);
}

/* debug-print an Eth header */
static void eth_debug_header(struct eth_header *hdr) {
    print_eth_addr("Dst MAC", hdr->dest_mac);
    print_eth_addr("Src MAC", hdr->src_mac);
    printf("EtherType: 0x%04x\n", ntohs(hdr->eth_type));
}

int ethernet_tx(struct pktbuf *pkt, const uint8_t *dst_mac, uint16_t ethertype) {
    struct eth_header *hdr;
    struct netdev *dev = netdev_get();

    // make room for the ethernet header
    hdr = pktbuf_push(pkt,sizeof(struct eth_header));
    if (!hdr) {
        eth_dbg("Failed to add Ethernet header");
        free_pktbuf(pkt);
        return -1;
    }

    // fill in the Ethernet header
    memcpy(hdr->dest_mac, dst_mac, 6);
    memcpy(hdr->src_mac, dev->hwaddr, 6);
    hdr->eth_type = htons(ethertype);

    // debug output
    eth_dbg("Sending Ethernet frame, type 0x%04x, length %d", ethertype, pkt->len);
    if (ETH_DEBUG) {
        eth_debug_header(hdr);
    }

    // transmit frame using network device
    return netdev_tx(pkt);
}

void ethernet_rx(struct pktbuf *pkt) {
    struct eth_header *hdr;
    uint16_t ethertype;

    // make sure we have at least enough data for an Eth header
    if (pkt->len < sizeof(struct eth_header)) {
        eth_dbg("Packet too short for Ethernet header (%d bytes)", pkt->len);
        free_pktbuf(pkt);
        return;
    }

    hdr = (struct eth_header *)pkt->data;
    ethertype = ntohs(hdr->eth_type);

    eth_dbg("Received Ethernet frame, type 0x%04x, length %d", ethertype, pkt->len);
    if (ETH_DEBUG) {
        eth_debug_header(hdr);
    }

    // remove the Eth header 
    pktbuf_pull(pkt, sizeof(struct eth_header));

    // set the protocol based on ethertype
    pkt->protocol = ethertype;

    // dispatch to the appropriate protocol handler
    switch (ethertype) {
        case ETH_P_ARP:
            eth_dbg("Dispatching ARP packet");
            arp_recv(pkt);
            break;
        case ETH_P_IP:
            eth_dbg("Dispatching IP packet");
            // ip_recv(pkt); uncomment when impl IP
            free_pktbuf(pkt); // leave until IP is impl
            break;
        default:
            eth_dbg("Unsupported ethertype 0x%04x", ethertype);
            free_pktbuf(pkt);
            break;
            
    }

}

void ethernet_init(void) {
    eth_dbg("Ethernet layer initialized");
}