#include <stdio.h>
#include <string.h> 
#include <arpa/inet.h>

#include "ip.h"

uint16_t checksum(void *addr, int count) {
    uint32_t sum = 0;
    uint16_t *ptr = addr;

    // add all 16-bit words
    while (count > 1) {
        sum += *ptr++;
        count -= 2;
    }

    // add any leftover odd byte
    if (count > 0) {
        sum += *((uint8_t*)ptr);
    }

    // Fold 32-bit sum into 16 bits
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    // take one's complement
    return ~sum;
}

int ip_validate_packet(struct ip_header *hdr, int len) {
    // check min length, make sure we have at least enough for basic header structure
    if (len < sizeof(struct ip_header)) {
        ip_dbg("Packet too short for IP header");
        return -1;
    }

    // verify versiom
    if (hdr->version != IPV4) {
        ip_dbg("Unsupported IP version %d", hdr->version);
        return -1;
    }

    // check IHL (header length), esnure hdr claims valid size for itself
    if (hdr->ihl < 5) {
        // standard size of IPv4 header with all required and no optional fields is 5 (4-bit) words
        ip_dbg("IP header length too small: %d", hdr->ihl);
        return -1;
    }

    // verify packet length, ensure packet's claimed total size doesnt exceed what we actually received
    uint16_t total_len = ntohs(hdr->len); 
    if (total_len > len) {
        ip_dbg("IP packet truncated, expected %d, got %d", total_len, len);
        return -1;
    }

    // verify checksum
    uint16_t orig_csum = hdr->csum;
    hdr->csum = 0; 
    uint16_t calc_csum = checksum(hdr, hdr->ihl * 4);
    hdr->csum = orig_csum;

    if (orig_csum != calc_csum) {
        ip_dbg("IP checksum mismatch: expected 0x%04x, calculated 0x%04x", ntohs(orig_csum), ntohs(calc_csum));
        return -1;
    }

    return 0;
}

void ip_init(void) {
    ip_dbg("IP layer initialized");
}
