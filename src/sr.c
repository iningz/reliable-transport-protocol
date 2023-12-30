#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../include/common.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#define TMO_INCR 20

struct snd_pkt {
    bool acked;
    int timeout;
    struct pkt pkt;
};

struct rcv_pkt {
    bool rcved;
    struct pkt pkt;
};

static struct {
    int base_seq;
    int next_seq;
    int win_size;
    int min_tmo;
    struct snd_pkt *snd_buf;
} sender;

static struct {
    int base_seq;
    int win_size;
    struct rcv_pkt *rcv_buf;
} receiver;

static void min_tmo_update()
{
    sender.min_tmo = INT_MAX;
    for (int i = sender.base_seq; i < sender.next_seq; i++) {
        struct snd_pkt *snd_pkt = &sender.snd_buf[i % sender.win_size];
        if (!snd_pkt->acked && snd_pkt->timeout < sender.min_tmo)
            sender.min_tmo = snd_pkt->timeout;
    }
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message) struct msg message;
{
    if (sender.next_seq >= sender.base_seq + sender.win_size) {
        msg_buf_push(&message);
        return;
    }

    struct snd_pkt *snd_pkt = &sender.snd_buf[sender.next_seq % sender.win_size];
    snd_pkt->acked = false;
    snd_pkt->timeout = get_sim_time() + TMO_INCR;
    if (snd_pkt->timeout < sender.min_tmo)
        sender.min_tmo = snd_pkt->timeout;

    struct pkt *pkt = &snd_pkt->pkt;
    memcpy(pkt->payload, message.data, sizeof(pkt->payload));
    pkt->seqnum = sender.next_seq;
    pkt->checksum = 0;
    pkt->checksum = get_checksum(pkt);
    tolayer3(A, *pkt);

    if (sender.base_seq == sender.next_seq)
        starttimer(A, TMO_INCR);
    sender.next_seq++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet) struct pkt packet;
{
    if (packet.acknum < sender.base_seq ||   //
        packet.acknum >= sender.next_seq ||  //
        get_checksum(&packet) != 0)
        return;

    struct snd_pkt *snd_pkt = &sender.snd_buf[packet.acknum % sender.win_size];
    snd_pkt->acked = true;
    while (sender.base_seq < sender.next_seq &&
           sender.snd_buf[sender.base_seq % sender.win_size].acked)
        sender.base_seq++;

    if (snd_pkt->timeout <= sender.min_tmo)
        min_tmo_update();

    stoptimer(A);
    if (sender.base_seq < sender.next_seq)
        starttimer(A, sender.min_tmo - get_sim_time());

    while (sender.next_seq < sender.base_seq + sender.win_size) {
        const struct msg *msg = msg_buf_pop();
        if (msg == NULL)
            break;
        A_output(*msg);
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    if (sender.base_seq == sender.next_seq)
        return;

    sender.min_tmo = INT_MAX;
    for (int i = sender.base_seq; i < sender.next_seq; i++) {
        struct snd_pkt *snd_pkt = &sender.snd_buf[i % sender.win_size];

        if (snd_pkt->acked)
            continue;

        if (snd_pkt->timeout <= get_sim_time()) {
            tolayer3(A, snd_pkt->pkt);
            snd_pkt->timeout = get_sim_time() + TMO_INCR;
        }

        if (snd_pkt->timeout < sender.min_tmo)
            sender.min_tmo = snd_pkt->timeout;
    }

    starttimer(A, sender.min_tmo - get_sim_time());
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    sender.base_seq = 0;
    sender.next_seq = 0;
    sender.win_size = getwinsize();
    sender.min_tmo = INT_MAX;
    sender.snd_buf = calloc(sender.win_size, sizeof(*sender.snd_buf));
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet) struct pkt packet;
{
    if (packet.seqnum < receiver.base_seq - receiver.win_size ||   //
        packet.seqnum >= receiver.base_seq + receiver.win_size ||  //
        get_checksum(&packet) != 0)
        return;

    struct rcv_pkt *rcv_pkt = &receiver.rcv_buf[packet.seqnum % receiver.win_size];
    struct pkt *ack_pkt = &rcv_pkt->pkt;
    ack_pkt->acknum = packet.seqnum;
    ack_pkt->checksum = 0;
    ack_pkt->checksum = get_checksum(ack_pkt);
    tolayer3(B, *ack_pkt);

    if (packet.seqnum < receiver.base_seq || rcv_pkt->rcved)
        return;

    memcpy(ack_pkt->payload, packet.payload, sizeof(ack_pkt->payload));
    receiver.rcv_buf[packet.seqnum % receiver.win_size].rcved = true;

    int idx = receiver.base_seq % receiver.win_size;
    while (receiver.rcv_buf[idx].rcved) {
        struct rcv_pkt *rcv_pkt = &receiver.rcv_buf[idx];
        tolayer5(B, rcv_pkt->pkt.payload);
        rcv_pkt->rcved = false;
        idx = ++receiver.base_seq % receiver.win_size;
    }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    receiver.base_seq = 0;
    receiver.win_size = getwinsize();
    receiver.rcv_buf = calloc(receiver.win_size, sizeof(*receiver.rcv_buf));
}
