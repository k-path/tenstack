#include <stdio.h>
#include <string.h> 
#include <arpa/inet.h>

#include "ip.h"
#include "netdev.h"
#include "icmp.h"


void ip_recv(struct pktbuf *pkt) {
    struct ip_header *hdr;
    struct netdev *dev = netdev_get();

    // make sure we have at least a basic IP header
    if (pkt->len < sizeof(struct ip_header)) {
        ip_dbg("Packet too short for IP header");
        free_pktbuf(pkt);
        return;
    }

    hdr = (struct ip_header *)pkt->data;

    // validate IP packet
    if (ip_validate_packet(hdr, pkt->len)) {
        ip_dbg("Invalid IP packet received");
        free_pktbuf(pkt);
        return;
    }

    // convert fields from network byte order
    uint32_t dst_addr = ntohl(hdr->daddr);
    
    // check if packet is for us
    if (dst_addr != ntohl(dev->addr)) {
        // if we were a router we could implement forwarding here
        ip_dbg("IP packet not for us, ignoring");
        free_pktbuf(pkt);
        return;
    }

    // remove IP header
    pktbuf_pull(pkt, hdr->ihl * 4);

    // process pkt based on the protocol
    switch (hdr->proto) {
        case IP_P_ICMP:
            ip_dbg("Dispatching ICMP packet");
            icmp_recv(pkt);
            break;
        case IP_P_TCP:
            ip_dbg("no TCP yet, drop");
            free_pktbuf(pkt);
            break;
        case IP_P_UDP:
            ip_dbg("no UDP yet, drop");
            free_pktbuf(pkt);
            break;
        default:
            ip_dbg("Unsupported protocol %d, dropping packet", hdr->proto);
            free_pktbuf(pkt);
            break;
    }
}   