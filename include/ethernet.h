#ifndef ETHERNET_H
#define ETHERNET_H

#include <linux/if_ether.h>
#include <stdint.h>


struct eth_header {
    uint8_t dest_mac[6]; // destination ether addr , 1 byte
    uint8_t src_mac[6]; // source ether addr
    uint16_t eth_type; // 2 bytes, packet type ID, indicates either length or type of the payload (e.g. IPv4, ARP), value is less than that it contains length of payload
} __attribute__((packed));


#endif 