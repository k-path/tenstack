#ifndef ICMP_H
#define ICMP_H
#include <stdint.h>
#include <pktbuf.h>

/* ICMP types */
#define ICMP_ECHO_REPLY 0 // when we sent ping
#define ICMP_DEST_UNREACHABLE 3
#define ICMP_ECHO_REQUEST 8 // when we receive ping

/* ICMP codes */
#define ICMP_NET_UNREACHABLE 0
#define ICMP_HOST_UNREACHABLE 1
#define ICMP_PORT_UNREACHABLE 3

struct icmp_v4 {
    uint8_t type; // purpose of message, ex. 0 (Echo Reply), 3 (Destination Unreachable), 8 (Echo Request)
    uint8_t code; // further describes meaning, can imply reason e.g., code 0 (Net Unreachable)
    uint16_t csum; // same algo as IP, end-to-end (payload included in calc) 
    uint8_t data[];
} __attribute__((packed));

struct icmp_v4_echo {
    uint16_t id; // set by sending host to determine which process echo reply is intended
    uint16_t seq; // sequence number, used to detect reordering or lost echo msgs
    uint8_t data[]; // optional, e.g., timestamp
} __attribute__((packed));

struct icmp_v4_dst_unreachable {
    uint8_t unused; // not used, completely no purpose, just need it
    uint8_t len;    // length of original datagram in 4-octet units for IPv4 (if 5, it means length is 5*4 = 20 bytes)
    uint16_t var;   // depends on ICMP code
    uint8_t data[]; // as much of the original IP packet as possible is placed here
} __attribute__((packed));
 
/* Process incoming ICMP packets */
void icmp_recv(struct pktbuf *pkt);

/* Send an ICMP Echo Request (ping) */
int icmp_send_echo_request(uint32_t dst_addr, uint16_t id, uint16_t seq);

#endif