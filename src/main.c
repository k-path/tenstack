#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>

#include "netdev.h"
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"

// flag to control program execution
static int running = 1;
static int seq = 0;

// sig handler for graceful shutdown
static void signal_handler(int signal) {
    printf("\nReceived signal %d, shutting down...\n", signal);
    running = 0;
}

int main(int argc, char *argv[]) {
    pthread_t rx_thread;

    // set up sig handling
    signal(SIGINT, signal_handler);

    printf("Starting TCP/IP stack...\n");

    netdev_init();
    ethernet_init();
    arp_init();
    ip_init();

    // create and config TAP device
    if (tapdev_init("tap0") < 0) {
        fprintf(stderr, "Failed to initialize TAP device\n");
        return EXIT_FAILURE;
    }

    // start packet rx thread
    if (pthread_create(&rx_thread, NULL, netdev_rx_loop, NULL) != 0) {
        perror("Failed to create RX thread");
        return EXIT_FAILURE;
    }

    printf("TCP/IP stack initialized, press Ctrl+C to cancel\n");

    // MAIN LOOP
    while (running) {
        // run ARP cache cleanup periodically
        arp_cache_timer();

        // ping test - send a ping every 3 seconds
        static time_t last_ping = 0;
        time_t now = time(NULL);
        
        if (now - last_ping >= 3) {
            char dst_str[INET_ADDRSTRLEN];
            uint32_t dst_addr = inet_addr("10.0.0.2");  // IP of TAP interface
            inet_ntop(AF_INET, &dst_addr, dst_str, INET_ADDRSTRLEN);
            
            printf("Sending ping to %s (seq=%d)\n", dst_str, seq);
            icmp_send_echo_request(dst_addr, 1234, seq++);
            last_ping = now;
        }

        // sleep for a while to avoid busy waiting
        sleep(1);
    }

    // ♫ clean up, everybody clean up ♪
    netdev_close();

    printf("TCP/IP stack shut down\n");
    return EXIT_SUCCESS;
}