#include <stdio.h>
#include <string.h> 
#include <arpa/inet.h>

#include "ip.h"
#include "netdev.h"
#include "icmp.h"
#include "ethernet.h"
#include "arp.h"

int ip_output(struct pktbuf *pkt, uint32_t dst_addr, uint8_t proto) {
    struct ip_header *iphdr;
    struct netdev *dev = netdev_get();
    static uint16_t ip_id = 0;
    uint8_t dst_mac[6];
    
    // create space for IP header
    iphdr = pktbuf_push(pkt, sizeof(struct ip_header));
    if (!iphdr) {
        ip_dbg("Failed to allocate space for IP header");
        free_pktbuf(pkt);
        return -1;
    }

    // fill in the IP header
    memset(iphdr, 0, sizeof(struct ip_header));

    iphdr->version = IPV4; 
    iphdr->ihl = 5; // 5 words, 20 bytes (standard IPV4 header), no options
    iphdr->tos = 0; // 0 is standard value for normal traffic
    iphdr->len = htons(pkt->len);
    iphdr->id = htons(ip_id++);
    iphdr->flags = 0;
    iphdr->frag_offset = 0;
    iphdr->ttl = IP_DEFAULT_TTL;
    iphdr->proto = proto;
    iphdr->saddr = dev->addr;
    iphdr->daddr = dst_addr;

    // calculate the IP header checksum
    iphdr->csum = 0;
    iphdr->csum = checksum(iphdr, sizeof(struct ip_header));

    // TODO: add MTU checking and IP fragmentation in more complete impl

    char dip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &dst_addr, dip_str, INET_ADDRSTRLEN);
    ip_dbg("Sending IP packet to %s, proto %d, len %d", dip_str, proto, pkt->len);

    // resolve MAC addr of destination/gateway, needs to be done before sending a packet
    if (arp_resolve(dst_addr, dst_mac) < 0) {
        ip_dbg("MAC resolution failed for %s, packet queued", dip_str);
        // TODO: queue packet and retry later
        free_pktbuf(pkt);
        return -1;
    }

    // transmit the packet using Ethernet
    return ethernet_tx(pkt, dst_mac, ETH_P_IP);
}

int ip_send(uint32_t dst_addr, uint8_t proto, void *data, int len) {
    struct pktbuf *pkt;

    // allocate a packet buffer with enough space for headers and data
    pkt = alloc_pktbuf(sizeof(struct eth_header) + sizeof(struct ip_header) + len);
    if (!pkt) {
        ip_dbg("Failed to allocate packet buffer");
        return -1;
    }

    // reserve space for headers (Ethernet + IP)
    pktbuf_reserve(pkt, sizeof(struct eth_header) + sizeof(struct ip_header));

    // copy the data into packet buffer
    void *pkt_data = pktbuf_put(pkt, len);
    memcpy(pkt_data, data, len);

    // send packet with full send logic
    return ip_output(pkt, dst_addr, proto);
}
