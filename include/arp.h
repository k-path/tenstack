#ifndef ARP_H
#define ARP_H

struct arp_header {
    unsigned short hwtype;  // hardware address type, determines link layer type used
    unsigned short protype; // protocol address type, IPv4, which is 0x0800
    unsigned char hwsize; // hardware address length, 6 bytes for MAC addresses, 4 bytes for IP addresses
    unsigned char prosize; // protocol address length
    unsigned short opcode; // ARP opcode(command), declares type of ARP message (ARP request (1), ARP reply (2), RARP request (3), RARP reply (4))
    unsigned char data[]; // arp data field, actual payload of ARP msg, will contain IPv4 specific info
} __attribute__((packed));

struct arp_ipv4 {
    unsigned char smac[6]; // 6 byte sender MAC addr
    unsigned int sip; // sender IP
    unsigned char dmac[6]; // 6 byte dest MAC addr
    unsigned int dip; // dest IP
} __attribute__((packed));

#endif