#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#include "icmp.h"
#include "ip.h"

#define icmp_dbg(fmt, ...) \
    printf("ICMP: " fmt "\n", ##__VA_ARGS__)

/* Process an echo request and send back an echo reply */
static int icmp_echo_reply(struct pktbuf *pkt) {
    struct pktbuf *reply;
    struct icmp_v4 *icmp_reply;
    struct icmp_v4 *icmp_request;
    struct icmp_v4_echo *echo_request, *echo_reply;
    int len, data_len;
    uint32_t src_addr;

    icmp_request = (struct icmp_v4 *)pkt->data;
    echo_request = (struct icmp_v4_echo *)icmp_request->data;

    // extract source address from the incoming packet
    src_addr = *((uint32_t *)(pkt->data - 8)); // minus 8 bytes to get source IP

    // calculate data length (total len - ICMP header - echo header)
    data_len = pkt->len - sizeof(struct icmp_v4) - sizeof(struct icmp_v4_echo);
    len = sizeof(struct icmp_v4) + sizeof(struct icmp_v4_echo) + data_len;

    // allocate new packet for reply
    reply = alloc_pktbuf(len);
    if (!reply) {
        icmp_dbg("Failed to allocate ICMP echo reply");
        return -1;
    }

    // fill ICMP header
    icmp_reply = (struct icmp_v4 *)pktbuf_put(reply, sizeof(struct icmp_v4));
    icmp_reply->type = ICMP_ECHO_REPLY;
    icmp_reply->code = 0; // standard code value when it's ECHO_REPLY
    icmp_reply->csum = 0;
    
    // fill in echo reply header
    echo_reply = (struct icmp_v4_echo *)pktbuf_put(reply, sizeof(struct icmp_v4_echo));
    echo_reply->id = echo_request->id;
    echo_reply->seq = echo_request->seq;

    // copy data from request to reply
    void *data = pktbuf_put(reply, data_len);
    memcpy(data, echo_request->data, data_len);

    // calc ICMP checksum
    icmp_reply->csum = checksum(icmp_reply, len);

    icmp_dbg("Sending ICMP Echo Reply, id=%d seq=%d", ntohs(echo_reply->id), ntohs(echo_reply->seq));

    // send ICMP reply
    return ip_send(src_addr, IP_P_ICMP, reply->data, reply->len);
}

void icmp_recv(struct pktbuf *pkt) {
    struct icmp_v4 *icmp;

    if (!pkt || pkt->len < sizeof(struct icmp_v4)) {
        icmp_dbg("Packet too small for ICMP header");
        if (pkt) free_pktbuf(pkt);
        return;
    }

    icmp = (struct icmp_v4 *)pkt->data;

    // validate checksum
    uint16_t csum = icmp->csum;
    icmp->csum = 0;
    if (checksum(icmp, pkt->len) != csum) {
        icmp_dbg("Invalid ICMP checksum");
        free_pktbuf(pkt);
        return;
    }
    icmp->csum = csum; // set csum back

    switch (icmp->type) {
        case ICMP_ECHO_REQUEST:
            icmp_dbg("Received ICMP Echo Request");
            if (icmp_echo_reply(pkt) < 0) {
                icmp_dbg("Failed to send ICMP Echo Reply");
            }
            break;
        
        case ICMP_ECHO_REPLY:
            // we'd handle logic to match the reply with requests here (for when we send out ping requests). 
            // can just Wireshark to test for now
            icmp_dbg("Received ICMP Echo Reply");
            break;

        case ICMP_DEST_UNREACHABLE:
            icmp_dbg("Received ICMP Destination Unreachable");
            // could notify upper layer protocols
            break;
        default:
            icmp_dbg("Unsupported ICMP type %d", icmp->type);
            break;
    }
    free_pktbuf(pkt);
}

int icmp_send_echo_request(uint32_t dst_addr, uint16_t id, uint16_t seq) {
    struct pktbuf *pkt;
    struct icmp_v4 *icmp;
    struct icmp_v4_echo *echo;
    char *data;
    int data_len = 56; // which is std ping data size
    int len = sizeof(struct icmp_v4) + sizeof(struct icmp_v4_echo) + data_len;

    // alloc packet buffer
    pkt = alloc_pktbuf(len);
    if (!pkt) {
        icmp_dbg("Failed to allocate packet for Echo Request");
        return -1;
    }

    // fill in ICMP header
    icmp = (struct icmp_v4 *)pktbuf_put(pkt, sizeof(struct icmp_v4));
    icmp->type = ICMP_ECHO_REQUEST;
    icmp->code = 0;
    icmp->csum = 0;

    // fill echo request header
    echo = (struct icmp_v4_echo *)pktbuf_put(pkt, sizeof(struct icmp_v4_echo));
    echo->id = htons(id);
    echo->seq = htons(seq);

    // add data - simple pattern
    data = pktbuf_put(pkt, data_len);
    int i;
    for (i = 0; i < data_len; i++) {
        data[i] = 'a' + (i % 26);
    }

    // calc checksum
    icmp->csum = checksum(icmp, len);

    icmp_dbg("Sending ICMP Echo Request to 0x%x, id=%d seq=%d", dst_addr, id, seq);

    // send ICMP packet
    return ip_send(dst_addr, IP_P_ICMP, pkt->data, pkt->len);
}