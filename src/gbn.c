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

/* called from layer 5, passed the data to be sent to other side */
static struct {
    int base_seq;
    int next_seq;
    int win_size;
    struct pkt *snd_buf;
} sender;

static struct {
    struct pkt ack_pkt;
} receiver;

/* called from layer 5, passed the data to be sent to other side */
void A_output(message) struct msg message;
{
    if (sender.next_seq >= sender.base_seq + sender.win_size) {
        msg_buf_push(&message);
        return;
    }

    struct pkt *pkt = &sender.snd_buf[sender.next_seq % sender.win_size];
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
    if (get_checksum(&packet) != 0)
        return;

    sender.base_seq = packet.acknum + 1;

    stoptimer(A);
    if (sender.base_seq < sender.next_seq)
        starttimer(A, TMO_INCR);

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

    starttimer(A, TMO_INCR);
    for (int i = sender.base_seq; i < sender.next_seq; i++) {
        struct pkt *snd_pkt = &sender.snd_buf[i % sender.win_size];
        tolayer3(A, *snd_pkt);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    sender.base_seq = 0;
    sender.next_seq = 0;
    sender.win_size = getwinsize();
    sender.snd_buf = calloc(sender.win_size, sizeof(*sender.snd_buf));
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet) struct pkt packet;
{
    if (packet.seqnum != receiver.ack_pkt.acknum + 1) {
        tolayer3(B, receiver.ack_pkt);
        return;
    }

    if (get_checksum(&packet) != 0)
        return;

    tolayer5(B, packet.payload);
    receiver.ack_pkt.acknum = packet.seqnum;
    receiver.ack_pkt.checksum = 0;
    receiver.ack_pkt.checksum = get_checksum(&receiver.ack_pkt);
    tolayer3(B, receiver.ack_pkt);
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    receiver.ack_pkt.acknum = -1;
}
