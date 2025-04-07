#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include "ethernet.h"

/*
 * alloc_tap: Initialize and configure a TAP device
 * @dev: Name of the TAP device (can be empty string for kernel to choose)
 * Returns: File descriptor for the TAP device or negative value on error
 */
int alloc_tap(char *dev) {
    struct ifreq ifr;
    int tapfd, err;

    // open the tun/tap device
    if ((tapfd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Cannot open TUN/TAP device\nMake sure one exists with '$ mknod /dev/net/tun c 10 200'");
        exit(1);
    }
    
    // initialize the interface request structure
    memset(&ifr, 0x0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    // copy device name to request if one is provided
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // configure the TUN/TAP device using ioctl
    if ( (err = ioctl(tapfd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ERR: Could not ioctl tun");
        close(tapfd);
        return err;
    }

    return tapfd;
}

// set IP address on the TAP device, set the nestmask, bring the interface up
int configure_tap(char *dev, char *ip, char *netmask) {
    char cmd[256];

    // set IP address and netmask
    snprintf(cmd, sizeof(cmd), "ip addr add %s/%s dev %s", ip, netmask, dev);
    system(cmd);

    // bring interface up
    snprintf(cmd, sizeof(cmd), "ip link set dev %s up", dev);
    system(cmd);

    return 0;
}

// read raw data from TAP device
int tap_read(int tapfd, unsigned char *buffer, int len) {
    return read(tapfd, buffer, len);
}

int main() {
    char tap_name[IFNAMSIZ] = "tap0";
    int tapfd;
    unsigned char buffer[2048];
    int nread;

    // allocate and configure tap device
    tapfd = alloc_tap(tap_name);
    configure_tap(tap_name, "192.168.1.1", "24");

    printf("TAP device %s initialized. Starting packet loop...\n", tap_name);

    while (1) {
        nread = tap_read(tapfd, buffer, sizeof(buffer));
        if (nread > 0) {
            printf("Received %d bytes\n", nread);
            // parse and process packet here
            struct eth_header *eth = (struct eth_header *)buffer;
            printf("Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                eth->src_mac[0], eth->src_mac[1], eth->src_mac[2], 
                eth->src_mac[3], eth->src_mac[4], eth->src_mac[5]);
            printf("Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                eth->dest_mac[0], eth->dest_mac[1], eth->dest_mac[2], 
                eth->dest_mac[3], eth->dest_mac[4], eth->dest_mac[5]);
            printf("EtherType: 0x%04x\n", ntohs(eth->eth_type));
             
             
        }
    }

    return 0;
    
}

// 
// int tap_write(int tapfd, unsigned char *buffer, int len) {
//     return write(tapfd, buffer, len);
// }

// int setup_tap_device() {

// }

// int close_tap() {

// }

