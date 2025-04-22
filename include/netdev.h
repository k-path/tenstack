#ifndef NETDEV_H
#define NETDEV_H

#include <stdint.h>
#include <linux/if.h>

#include "pktbuf.h"

#define NETDEV_MTU 1500 // Default MTU 

/* Single global network device structure */
struct netdev {
    uint8_t hwaddr[6];      // MAC address
    uint32_t addr;          // IPv4 address
    uint32_t netmask;       // Network mask
    char name[IFNAMSIZ]; // Interface name ()
    int mtu;                // maximum transimission unit
};

/* Tap device structure */
struct tapdev {
    struct netdev dev; // Embedded network device
    int fd;            // tap device file descriptor
};

/* Single global TAP device */
extern struct tapdev tap;

/* Initialize a network device */
void netdev_init(void);

/* Initialize and open the TAP device */
int tapdev_init(const char *name);

/* Transmit a packet through the network device */
int netdev_tx(struct pktbuf *pkt);

/* Poll for incoming packets (non-blocking) */
int netdev_poll(void);

/* Dedicated thread function to receive packets */
void *netdev_rx_loop(void *arg);

/* Clean up resources */
void netdev_close(void);

/* Function to retrieve a network device */
struct netdev *netdev_get(void);

#endif 
