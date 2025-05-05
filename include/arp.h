#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include "ethernet.h"
#include "list.h"

#define ARP_HW_ETHERNET 1 // Ethernet hardware type
#define ARP_OP_REQUEST  1 // ARP request
#define ARP_OP_REPLY    2 // ARP reply

/* ARP entry states */
#define ARP_FREE       0 // slot is unused
#define ARP_WAITING    1 // ARP request sent but no reply yet
#define ARP_RESOLVED   2 // valid mapping

#define ARP_CACHE_TTL  60 // 1 min timeout 

/* ARP packet format */
struct arp_header {
    uint16_t hwtype;  // hardware address type, determines link layer type used (ethernet, point-to-point, etc)
    uint16_t protype; // protocol address type, IPv4, which is 0x0800
    uint8_t hwlen; // hardware address length, 6 bytes for MAC addresses, 4 bytes for IP addresses
    uint8_t prolen; // protocol address length
    uint16_t opcode; // ARP opcode(command), declares type of ARP message (ARP request (1), ARP reply (2), RARP request (3), RARP reply (4))
    uint8_t data[]; // arp data field, actual payload of ARP msg, will contain IPv4 info specifically
} __attribute__((packed));

/* IPv4 information */
struct arp_ipv4 {
    uint8_t smac[6]; // 6 byte sender MAC addr
    uint32_t sip; // sender IP
    uint8_t dmac[6]; // 6 byte dest MAC addr
    uint32_t dip; // dest IP
} __attribute__((packed));

/* ARP Cache to store IP-to-MAC mappings */
struct arp_cache_entry {
    list_head ace_list;
    uint32_t ip; // IP address
    uint8_t mac[6]; // hardware address
    int ttl; // entry timeout
    int state; // state of the entry (complete, incomplete)
};

/* Initialize ARP module */
int arp_init(void);

/* creates and sends an ARP request packet */
int arp_request(uint32_t dip);

/* process incoming ARP packets */
void arp_process(struct arp_header *hdr, int len);

/* update ARP cache with new IP-to-MAC mapping  */
void arp_update_cache(uint32_t ip, uint8_t *mac);

/* Clean up expired ARP cache entries */
void arp_cache_timer(void);

/* resolves an IP address to a MAC address for sending packets */
int arp_resolve(uint32_t ip, uint8_t *mac);

/* handles ethernet frames with ARP EtherType */
void arp_recv(struct pktbuf *pkt);

#endif /* ARP.H */