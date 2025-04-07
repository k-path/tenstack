#ifndef ETHERNET_H
#define ETHERNET_H

#include <linux/if_ether.h>


struct eth_header {
    unsigned char dest_mac[6]; // destination ether addr
    unsigned char src_mac[6]; // source ether addr
    unsigned short eth_type; // packet type ID
} __attribute__((packed));



#endif 