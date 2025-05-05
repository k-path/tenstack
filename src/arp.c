#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "arp.h"
#include "netdev.h"
#include "pktbuf.h"

/* Global ARP cache with mutex protection */
static LIST_HEAD(arp_cache);
static pthread_mutex_t arp_cache_lock = PTHREAD_MUTEX_INITIALIZER;
static const uint8_t ETH_BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // ethernet broadcast address, meaning send to all devices on local network

/* Debug output macro */
#define arp_dbg(fmt, ...) \
    printf("ARP: " fmt "\n", ##__VA_ARGS__)

/* Print IP address in dotted notation */
/* static void print_ip(const char *name, uint32_t ip) {
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0XFF;
    bytes[2] = (ip >> 16) & 0XFF;
    bytes[3] = (ip >> 24) & 0XFF;

    printf("%s: %d.%d.%d.%d\n", name, bytes[0], bytes[1], bytes[2], bytes[3]);
} */

int arp_init(void) {
    // arp_cache already initialized by LIST_HEAD
    arp_dbg("ARP module initialized\n");
    return 0;
}

/* Create a new ARP cache entry */
static struct arp_cache_entry *arp_cache_entry_create(uint32_t ip, uint8_t *mac) {
    struct arp_cache_entry *entry = malloc(sizeof(struct arp_cache_entry)); 
    if (!entry) {
        perror("Failed to allocate ARP cache entry");
        return NULL;
    }
    list_init(&entry->ace_list);

    entry->ip = ip;
    memcpy(entry->mac, mac, sizeof(entry->mac));
    entry->ttl = ARP_CACHE_TTL; 
    entry->state = ARP_RESOLVED;

    return entry;
}

/* Find an entry in the ARP cache by IP address */
static struct arp_cache_entry *arp_cache_lookup(uint32_t ip) {
    struct arp_cache_entry *entry; // curr cache entry we're examining
    list_head *elem; // pointer that traverses through linked list nodes
    
    // traverse the list for a matching IP
    list_for_each(elem, &arp_cache) {
        entry = list_entry(elem, struct arp_cache_entry, ace_list); // convert from list_head pointer to containing struct (arp_cache_entry)
        if (entry->ip == ip) {
            return entry; // found match
        }
    }

    return NULL; // no match found
}

void arp_update_cache(uint32_t ip, uint8_t *mac) {
    struct arp_cache_entry *entry;

    // acquire mutex lock safely to prevent concurrent modifying
    pthread_mutex_lock(&arp_cache_lock); 
    char ip_str[INET_ADDRSTRLEN]; // buffer for ip string representation

    // convert ip to string 
    inet_ntop(AF_INET, &ip, ip_str, INET_ADDRSTRLEN);
    
    // check if entry already exists first
    entry = arp_cache_lookup(ip);

    if (entry) {
        // update existing entry
        memcpy(entry->mac, mac, sizeof(entry->mac));
        entry->ttl = ARP_CACHE_TTL; // reset ttl
        entry->state = ARP_RESOLVED;
        arp_dbg("Updated ARP cache entry for IP %s", ip_str);
    } else {
        // create new entry
        entry = arp_cache_entry_create(ip, mac);
        if (entry) {
            // Add to cache
            list_add(&entry->ace_list, &arp_cache);
            arp_dbg("Added new ARP cache entry for IP %s", ip_str);
        }
    }

    pthread_mutex_unlock(&arp_cache_lock);
}

/* Clean up expired ARP cache entries */
void arp_cache_timer(void) {
    struct arp_cache_entry *entry;
    list_head *elem, *tmp;
    char ip_str[INET_ADDRSTRLEN]; 
    
    pthread_mutex_lock(&arp_cache_lock);
    
    list_for_each_safe(elem, tmp, &arp_cache) {
        entry = list_entry(elem, struct arp_cache_entry, ace_list);

        // decrement TTL
        entry->ttl--;

        // remove if expired
        if (entry->ttl <= 0) {
            inet_ntop(AF_INET, &entry->ip, ip_str, INET_ADDRSTRLEN);
            arp_dbg("Removing expired ARP entry for IP %s", ip_str);

            list_del(elem); // remove from list
            free(entry); // free memory
        }
    }

    pthread_mutex_unlock(&arp_cache_lock);
}

int arp_request(uint32_t dip) {
    struct pktbuf *pkt;
    struct arp_header *arp;
    struct arp_ipv4 *arp_data;
    struct netdev *dev = netdev_get();
    char ip_str[INET_ADDRSTRLEN]; 

    // create a packet buffer for the ARP request
    pkt = alloc_pktbuf(sizeof(struct eth_header) + sizeof(struct arp_header) + sizeof(struct arp_ipv4));
    // reserve space for the Ethernet header
    pktbuf_reserve(pkt, sizeof(struct eth_header));
    if (!pkt) {
        arp_dbg("Failed to allocate packet buffer for ARP request");
        return -1;
    }

    // make room for ARP header AND data
    arp = pktbuf_put(pkt, sizeof(struct arp_header) + sizeof(struct arp_ipv4)); // uses put since we're adding both header and data (data goes last)

    // fill in the arp header, convert to network byte order if applicable
    arp->hwtype =  htons(ARP_HW_ETHERNET);  // hardware type is eth
    arp->protype = htons(ETH_P_IP);         // protocol type is IPv4
    arp->hwlen = 6;                         // mac addr len
    arp->prolen = 4;                        // IPv4 addr length
    arp->opcode = htons(ARP_OP_REQUEST);    // ARP request

    // fill in the ARP data
    arp_data = (struct arp_ipv4 *)arp->data;
    
    // set sender MAC and IP
    memcpy(arp_data->smac, dev->hwaddr, 6);
    arp_data->sip = dev->addr;

    // set target's, MAC is zero when requesting (since it's what we're trying to discover)
    memset(arp_data->dmac, 0, 6);
    arp_data->dip = dip;  

    inet_ntop(AF_INET, &dip, ip_str, INET_ADDRSTRLEN);
    arp_dbg("Sending ARP request for IP %s", ip_str);

    // send the ARP request as an Ethernet frame to the broadcast address
    return ethernet_tx(pkt, ETH_BROADCAST_ADDR, ETH_P_ARP);
}

void arp_process(struct arp_header *hdr, int len) {
    struct arp_ipv4 *arp_data;
    struct netdev *dev = netdev_get();
    uint16_t opcode;

    if (len < sizeof(struct arp_header) + sizeof(struct arp_ipv4)) {
        arp_dbg("ARP packet too short");
        return;
    }

    // ensure this is an Ethernet/Ipv4 ARP packet
    if (ntohs(hdr->hwtype) != ARP_HW_ETHERNET || 
        ntohs(hdr->protype) != ETH_P_IP || hdr->hwlen != 6 || hdr->prolen != 4) {
        arp_dbg("Unsupported ARP packet format");
        return;
    }

    opcode = ntohs(hdr->opcode);
    arp_data = (struct arp_ipv4 *)hdr->data;

    // update ARP cache with sender's info regardless of packet type
    arp_update_cache(arp_data->sip, arp_data->smac);

    arp_dbg("Processed ARP packet, opcode: %d", opcode);

    if (opcode == ARP_OP_REQUEST) {
        // char our_ip_str[INET_ADDRSTRLEN], target_ip_str[INET_ADDRSTRLEN];
        // inet_ntop(AF_INET, &dev->addr, our_ip_str, INET_ADDRSTRLEN);
        // inet_ntop(AF_INET, &arp_data->dip, target_ip_str, INET_ADDRSTRLEN);
        // arp_dbg("Our IP: %s, Target IP: %s", our_ip_str, target_ip_str);
        
        // check if the request is for our IP
        if (arp_data->dip != dev->addr) {
            arp_dbg("ARP request not for us, ignoring");
            return;
        }

        arp_dbg("Received ARP request for our IP, sending reply");

        // Send an ARP REPLY:
        // create a packet buffer for the ARP reply
        struct pktbuf *pkt = alloc_pktbuf(sizeof(struct eth_header) + sizeof(struct arp_header) + sizeof(struct arp_ipv4));
        // make rooom for Ethernet header
        pktbuf_reserve(pkt, sizeof(struct eth_header));
        if (!pkt) {
            arp_dbg("Failed to allocate packet buffer for ARP reply");
            return;
        }

        // make room for ARP header and data
        struct arp_header *arp = pktbuf_put(pkt, sizeof(struct arp_header) + sizeof(struct arp_ipv4));
        struct arp_ipv4 *reply_data = (struct arp_ipv4 *)arp->data;

        // fill in the ARP header
        arp->hwtype =  htons(ARP_HW_ETHERNET);
        arp->protype = htons(ETH_P_IP);         
        arp->hwlen = 6;                         
        arp->prolen = 4;                        
        arp->opcode = htons(ARP_OP_REPLY);

        // fill in ARP data for reply
        // our MAC and IP (sender)
        memcpy(reply_data->smac, dev->hwaddr, 6);
        reply_data->sip = dev->addr;

        // original requester's MAC and IP (dest)
        memcpy(reply_data->dmac, arp_data->smac, 6);
        reply_data->dip = arp_data->sip;

        // send the ARP reply directly to requester
        ethernet_tx(pkt, arp_data->smac, ETH_P_ARP);
    }
}

int arp_resolve(uint32_t ip, uint8_t *mac) {
    struct arp_cache_entry *entry;

    // lock cache while accessing it
    pthread_mutex_lock(&arp_cache_lock);

    entry = arp_cache_lookup(ip);

    if (entry && entry->state == ARP_RESOLVED) {
        // copy the MAC
        memcpy(mac, entry->mac, 6);
        pthread_mutex_unlock(&arp_cache_lock); // dont forget to unlock
        return 0;
    }

    pthread_mutex_unlock(&arp_cache_lock); // dont forget to unlock

    // if not found or waiting, send ARP request
    arp_request(ip);

    return -1; // not resolved yet
}

void arp_recv(struct pktbuf *pkt) {
    struct arp_header *hdr;

    // ensure packet is valid
    if (!pkt || pkt->len < sizeof(struct arp_header)) {
        arp_dbg("Invalid ARP packet received");
        if (pkt) free_pktbuf(pkt);
        return;
    }

    hdr = (struct arp_header *)pkt->data;

    // process the ARP packet
    // extract ARP data for better debugging
    // if (pkt->len >= sizeof(struct arp_header) + sizeof(struct arp_ipv4)) {
    //     struct arp_ipv4 *arp_data = (struct arp_ipv4 *)hdr->data;
    //     char sip_str[INET_ADDRSTRLEN], dip_str[INET_ADDRSTRLEN];
        
    //     inet_ntop(AF_INET, &arp_data->sip, sip_str, INET_ADDRSTRLEN);
    //     inet_ntop(AF_INET, &arp_data->dip, dip_str, INET_ADDRSTRLEN);
        
    //     arp_dbg("Received ARP packet, opcode: %d, source IP: %s, target IP: %s", 
    //             ntohs(hdr->opcode), sip_str, dip_str);
    // }
    arp_process(hdr, pkt->len);


    free_pktbuf(pkt);
}