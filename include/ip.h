#ifndef IPV4_H
#define IPV4_H
#include <stdint.h>
#include "pktbuf.h"

#define IPV4 4

/* IP header flags */
#define IP_RF 0x8000 // reserved, must be zero
#define IP_DF 0x4000 // don't fragment
#define IP_MF 0x2000 // more fragments
// occupy the first 3 bits of the 16 bit field, frag_offset occupies the rest

/* IP protocols */
#define IP_P_ICMP 1
#define IP_P_TCP 6
#define IP_P_UDP 17

#define IP_DEFAULT_TTL 64

/* Debug output macro */
#define ip_dbg(fmt, ...) \
    printf("IP: " fmt "\n", ##__VA_ARGS__)

struct ip_header {
    uint8_t ihl : 4;     // number of 32-bit words in the IP header. These two lines pack two 4-bit fields into a single byte
    uint8_t version : 4; // format of the Internet header 
    uint8_t tos;         // type of service
    uint16_t len;        // total length of the whole IP datagram
    uint16_t id;         // used to index the datagram, used for reassembly of fragmented IP datagrams (determine fragments go together)
    uint16_t flags : 3;  // control flags of the datagram (fragment allowed, last fragment, more incoming fragments)
    uint16_t frag_offset : 13; // position of the fragment in the datagram (helps determine order)
    uint8_t ttl;          // time to live
    uint8_t proto;        // protocol, (UDP, TCP)
    uint16_t csum;        // checksum, verify integrity of header
    uint32_t saddr;       // source address of datagram
    uint32_t daddr;       // destination address of datagram
} __attribute__((packed));

/* Calculate IP checksum, works on any header */
uint16_t checksum(void *addr, int count);

/* Validate an IP packet */
int ip_validate_packet(struct ip_header *hdr, int len);

/* Initialize the IP subsystem */
void ip_init(void);

/* Process incoming IP packets */
void ip_recv(struct pktbuf *pkt);

/* Build and transmit IP packet */
int ip_output(struct pktbuf *pkt, uint32_t dst_addr, uint8_t proto);

/* Send a raw IP packet with provided data */
int ip_send(uint32_t dst_addr, uint8_t proto, void *data, int len);

#endif 