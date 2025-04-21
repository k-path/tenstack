#ifndef PKTBUF_H
#define PKTBUF_H

#include <stdint.h>
#include "list.h"

/* Packet buffer structure */
struct pktbuf {
    list_head list;     // For queueing packets, allows us to chain packets together
    uint8_t *data;      // Pointer to the actual packet data, this pointer moves as we manipulate packet
    uint8_t *head;      // Start of the buffer
    uint8_t *end;       // end of allocated buffer space (head + size)
    uint32_t size;      // Total buffer size
    uint32_t len;       // Current data length
    uint16_t protocol;  // Protocol identifier (ETH_P_IP, ETH_P_ARP, etc.) 
    int refcnt;         // reference count

    struct netdev *dev; // Reference to the network device
};


/* Allocate a new packet buffer with specified cap */
struct pktbuf *alloc_pktbuf(uint32_t size);

/* Free a packet buffer */
void free_pktbuf(struct pktbuf *pkt);

/* Increment the reference count for a packet buffer */
void pktbuf_hold(struct pktbuf *pkt);

/* Reserve space at the beginning of the buffer (for headers) */
int pktbuf_reserve(struct pktbuf *pkt, uint32_t len);

/* Add data to the start of the buffer (used for adding headers) */
void *pktbuf_push(struct pktbuf *pkt, uint32_t len);

/* Remove data from the start of the buffer (used when processing headers) */
void *pktbuf_pull(struct pktbuf *pkt, uint32_t len);

/* Add data to the end of the buffer */
void *pktbuf_put(struct pktbuf *pkt, uint32_t len);

/* Clone a packet buffer */
struct pktbuf *pktbuf_clone(struct pktbuf *pkt);

#endif
