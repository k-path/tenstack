#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pktbuf.h"


struct pktbuf *alloc_pktbuf(uint32_t size) {
    struct pktbuf *pkt = malloc(sizeof(struct pktbuf));

    if (!pkt) {
        perror("Failed to allocate packet buffer");
        return NULL;
    }

    // initialize the packet structure 
    memset(pkt, 0, sizeof(struct pktbuf)); // zero out
    list_init(&pkt->list); // initialize list field

    // allocate memory for the buffer 
    pkt->head = malloc(size);
    if (!pkt->head) {
        free(pkt);
        return NULL;
    }

    // initialize fields
    pkt->data = pkt->head;
    pkt->size = size;
    pkt->len = 0;
    pkt->refcnt = 1;
    pkt->end = pkt->head + size;

    return pkt;
}

void free_pktbuf(struct pktbuf *pkt) {
    if (!pkt) {
        return;
    }

    pkt->refcnt--;

    if (pkt->refcnt == 0) {
        if (pkt->head) {
            free(pkt->head);
        }

        free(pkt);
    }

    // if refcnt still >0, buffer still in use somewhere
}

void pktbuf_hold(struct pktbuf *pkt) {
    if (pkt) {
        pkt->refcnt++;
    }
}

int pktbuf_reserve(struct pktbuf *pkt, uint32_t len) {
    if (!pkt || len > pkt->size) {
        return -1;
    }

    pkt->data = pkt->head + len; // absolute offset from head
    return 0;
}

void *pktbuf_push(struct pktbuf *pkt, uint32_t len) {
    if (!pkt || pkt->data - len < pkt->head) {
        return NULL;
    }

    pkt->data -= len; // move data ptr back, adding layer header
    pkt->len += len;

    return pkt->data;
}

void *pktbuf_pull(struct pktbuf *pkt, uint32_t len) {
    if (!pkt || len > pkt->len) {
        return NULL;
    }

    void *data = pkt->data; // get header data

    pkt->data += len;
    pkt->len -= len; // remove layer header

    return data;
}

void *pktbuf_put(struct pktbuf *pkt, uint32_t len) {
    if (!pkt || pkt->data + pkt->len + len > pkt->end) {
        return NULL;
    }

    void *data = pkt->data + pkt->len; // calc where new data should be placed
    pkt->len += len;

    return data; // write new data here
} 

struct pktbuf *pktbuf_clone(struct pktbuf *pkt) {
    if (!pkt) return NULL;
    
    struct pktbuf *clone = alloc_pktbuf(pkt->size);
    if (!clone) return NULL;

    // copy packet metadata 
    clone->len = pkt->len;
    clone->protocol = pkt->protocol;
    clone->dev = pkt->dev;

    // calculate the data offset 
    uint32_t offset = pkt->data - pkt->head;
    clone->data = clone->head + offset;

    // Copy the actual data 
    memcpy(clone->head, pkt->head, pkt->size);

    return clone;
}