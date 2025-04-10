#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include "ethernet.h"
#include "utils.h"



/*
 * alloc_tap: Initialize and configure a TAP device
 * @dev: Name of the TAP device (can be empty string for kernel to choose)
 * Returns: File descriptor for the TAP device or negative value on error
 */
int alloc_tap(char *dev) {
    struct ifreq ifr;
    int tapfd;

    // open the tun/tap device
    if ((tapfd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Cannot open TUN/TAP device\nMake sure one exists with '$ mknod /dev/net/tun c 10 200'");
        return -1; // return -1 instead of exit(1) since this is a utility function which shouldnt make decisions abt terminating the entire program
    }
    
    // initialize the interface request structure
    memset(&ifr, 0x0, sizeof(ifr));

     // IFF_TAP = Layer 2 (Ethernet) device, IFF_NO_PI = No extra packet info
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    // copy device name to request, if one is provided
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // configure the TUN/TAP device using ioctl
    if (ioctl(tapfd, TUNSETIFF, (void *)&ifr) < 0) {
        perror("ERR: Could not ioctl tun");
        close(tapfd);
        return -1;
    }
    
    // copy device name (handles case where device name not specified and kernel assigned one)
    strncpy(dev, ifr.ifr_name, IFNAMSIZ);
    return tapfd;
}

/* sets IP address on the TAP if */
int set_ipaddr(char *dev, char *cidr) {
    return run_cmd("ip addr add %s dev %s", cidr, dev);
}

/* adds a route via TAP if */
int set_route(char *dev, char *cidr) {
    return run_cmd("ip route add %s dev %s", cidr, dev);
}

/* Brings up the TAP if */
int set_up_if(char *dev) {
    return run_cmd("ip link set dev %s up", dev);
}

int configure_tap(char *dev, char *cidr) {
    // sets up IP address on TAP device
    if (set_ipaddr(dev, cidr) != 0) {
        fprintf(stderr,"Error setting TAP address\n");
    }

    // brings up the interface
    if (set_up_if(dev) != 0) {
        fprintf(stderr, "Error bringing up TAP interface\n");
    }
    
    /* set_route(dev, cidr); // not adding by default */  
    return 0;
}

int setup_network_if(char *tap_name, int choose_name, char *cidr) {
    char dev[IFNAMSIZ];
    int tapfd;

    // if specific name is requested, use it, or we just let kernel choose
    if (!choose_name && tap_name) {
        strncpy(dev, tap_name, IFNAMSIZ - 1);
        dev[IFNAMSIZ - 1] = '\0'; // make sure to null terminate
    } else {
        dev[0] = '\0';
    }

    // create TAP interface
    tapfd = alloc_tap(dev);
    if (tapfd < 0) {
        fprintf(stderr, "Failed to create TAP device\n");
        return -1;
    }

    printf("TAP device %s initialized\n", dev);

    // configure the interface
    if (configure_tap(dev, cidr) != 0) {
        fprintf(stderr, "Failed to configure TAP device\n");
        close(tapfd);
        return -1;
    }

    return tapfd;
}

// read raw data from TAP device
int tap_read(int tapfd, unsigned char *buffer, int len) {
    return read(tapfd, buffer, len);
}

// 
// int tap_write(int tapfd, unsigned char *buffer, int len) {
//     return write(tapfd, buffer, len);
// }

int main() {
    int tapfd;
    char tap_name[IFNAMSIZ] = "tap0";

    tapfd = setup_network_if(tap_name, 0, "192.168.1.1/24");
    unsigned char buffer[2048];
    int nread;

    printf("Starting packet loop...\n");

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



// int close_tap() {

// }

