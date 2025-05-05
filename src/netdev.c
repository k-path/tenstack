#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <errno.h>

#include "netdev.h"
#include "tap.h"
#include "ethernet.h"
#include "pktbuf.h"
#include "arp.h"
#include "utils.h"

/* Global TAP device instance */
struct tapdev tap;

/* Flag to control RX loop */
static int running = 0;

/* Debug output for network devices */
#define netdev_dbg(fmt, ...) \
    printf("NETDEV: " fmt "\n", ##__VA_ARGS__)

void netdev_init(void) {
    memset(&tap, 0, sizeof(tap));


    char hwaddr[] = "02:42:AC:11:00:02"; 
    sscanf(hwaddr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        &tap.dev.hwaddr[0],
        &tap.dev.hwaddr[1],
        &tap.dev.hwaddr[2],
        &tap.dev.hwaddr[3],
        &tap.dev.hwaddr[4],
        &tap.dev.hwaddr[5]);

    // set IP address and netmask 
    if (inet_pton(AF_INET, "10.0.0.1", &tap.dev.addr) <= 0) {
        fprintf(stderr, "Failed to convert IP address\n");
        return;
    }

    if (inet_pton(AF_INET, "255.255.255.0", &tap.dev.netmask) <= 0) {
        fprintf(stderr, "Failed to convert netmask\n");
        return;
    }

    tap.dev.mtu = NETDEV_MTU;

    netdev_dbg("Network subsystem intialized");
}

/* Initialize and open the TAP interface */
int tapdev_init(const char *name) {
    char *cidr = "10.0.0.2/24";
    char dev[IFNAMSIZ];

    // copy name to buffer 
    strncpy(dev, name, IFNAMSIZ - 1);
    dev[IFNAMSIZ - 1] = '\0';


    tap.fd = setup_network_if(dev, 0, cidr);
    if (tap.fd < 0) {
        return -1;
    }

    // set up networking device structure
    strncpy(tap.dev.name, dev, IFNAMSIZ - 1);

    running = 1;
    return 0;
}

int netdev_tx(struct pktbuf *pkt) {
    if (!pkt || !pkt->data || pkt->len == 0) {
        netdev_dbg("Invalid packet for transmission");
        return -1;
    }

    netdev_dbg("Transmitting packet of %d bytes", pkt->len);

    // write the packet to TAP device
    int ret = tap_write(tap.fd, pkt->data, pkt->len);

    if (ret < 0) {
        perror("Error writing to TAP device");
    }

    netdev_dbg("Successfully transmitted %d bytes", ret);
    return ret;
}

int netdev_poll(void) {
    // allocate a packet buffer with extra room for headers
    struct pktbuf *pkt = alloc_pktbuf(NETDEV_MTU + 100); 
    if (!pkt) {
        fprintf(stderr, "Failed to allocate packet buffer\n");
        return -1;
    }

    // read directly into pktbuf data 
    int nread = tap_read(tap.fd, pkt->data, pkt->size);

    if (nread > 0) {
        netdev_dbg("Received %d bytes", nread);

        // update the length field to match what we read
        pkt->len = nread;

        // set the device
        pkt->dev = &tap.dev;

        // process the Ethernet frame
        ethernet_rx(pkt);

        return 1;
    } else {
        // no data or some error occurred
        free_pktbuf(pkt); 

        if (nread < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Error reading from TAP device");
            return -1;
        }

        return 0; // no packets available
    }
}

void *netdev_rx_loop(void *arg){
    netdev_dbg("RX thread starting");

    while (running) {
        // poll for packets
        netdev_poll();

        // sleep to avoid consuming too much CPU
        usleep(1000);
        
    }
    netdev_dbg("RX thread exiting");
    return NULL;
}

void netdev_close(void) {
    // signal RX thread to stop
    running = 0;

    // close TAP dev 
    if (tap.fd >= 0) {
        close_tap(tap.fd);
        tap.fd = -1;
    }

    netdev_dbg("Network device resources cleaned up");
}

struct netdev *netdev_get(void) {
    return &tap.dev;
}