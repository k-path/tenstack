#include "arp.h"
#include <pthread.h>

/* Global ARP cache with mutex protection */
static LIST_HEAD(arp_cache);
static pthread_mutex_t arp_cache_lock = PTHREAD_MUTEX_INITIALIZER;
static const uint8_t ETH_BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // ethernet broadcast address, meaning send to all devices on local network


/* Initialize ARP module */
int arp_init(void) {
    // arp_cache already initialized by LIST_HEAD
    printf("ARP module initialized\n");
    return 0;
}

/* Create a new ARP cache entry */
static struct arp_cache_entry *arp_cache_entry_create(uint32_t ip, uint8_t *mac) {
    struct arp_cache_entry *entry = malloc(sizeof(struct arp_cache_entry)); 
    if (!entry) {
        perror("Failed to allocate ARP cache entry");
        return NULL;
    }

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

    return NULL;
}

void arp_update_cache(uint32_t ip, uint8_t *mac) {
    struct arp_cache_entry *entry;

    // acquire mutex lock safely to prevent concurrent modifying
    pthread_mutex_lock(&arp_cache_lock); 

    // check if entry already exists first
    entry = arp_cache_lookup(ip);

    if (entry) {
        // update existing entry
        memcpy(entry->mac, mac, sizeof(mac));
        entry->ttl = ARP_CACHE_TTL; // reset ttl
        entry->state = ARP_RESOLVED;
    } else {
        // create new entry
        entry = arp_cache_entry_create(ip, mac);
        if (entry) {
            // Add to cache
            list_add(&entry->ace_list, &arp_cache); 
        }
    }

    pthread_mutex_unlock(&arp_cache_lock);
}

/* Clean up expired ARP cache entries */
void arp_cache_timer(void) {
    struct arp_cache_entry *entry;
    list_head *elem, *tmp;

    pthread_mutex_lock(&arp_cache_lock);

    list_for_each_safe(elem, tmp, &arp_cache) {
        entry = list_entry(elem, struct arp_cache_entry, ace_list);

        // decrement TTL
        entry->ttl--;

        // remove if expired
        if (entry->ttl <= 0) {
            list_del(elem); // remove from list
            free(entry); // free memory
        }
    }

    pthread_mutex_lock(&arp_cache_lock);
}

/* Create and send an ARP request packet */
int arp_request(uint32_t dip) {
    // Allocate buffer for Ethernet frame + ARP header + ARP data
    uint8_t buffer[ETH_FRAME_LEN];
    struct eth_header *eth = (struct eth_header *)buffer; // buffer's first bytes is Eternet header
    struct arp_header *arp = (struct arp_header *)(buffer + sizeof(struct eth_header)); // next set of bytes is arp header
    struct arp_ipv4 *arp_data = (struct arp_ipv4 *)arp->data; 
    // so data will be laid out as valid eth frame containing an ARP packet


    // initialize Ethernet header






}