#ifndef ETHERNET_H
#define ETHERNET_H

#include <linux/if_ether.h>


struct eth_header {
    unsigned char dest_mac[6]; // destination ether addr , 1 byte
    unsigned char src_mac[6]; // source ether addr
    unsigned short eth_type; // 2 bytes, packet type ID, indicates either length or type of the payload (e.g. IPv4, ARP), value is less than that it contains length of payload
} __attribute__((packed));



#endif 