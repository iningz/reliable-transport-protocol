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

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/
#define TMO_INCR 10

static struct {
    enum { SENDER_WAIT_CALL, SENDER_WAIT_ACK } state;
    int seqnum;
    struct pkt snd_pkt;
} sender;

static struct {
    int seqnum;
} receiver;

/* called from layer 5, passed the data to be sent to other side */
void A_output(message) struct msg message;
{
    if (sender.state != SENDER_WAIT_CALL) {
        msg_buf_push(&message);
        return;
    }

    memcpy(sender.snd_pkt.payload, message.data, sizeof(sender.snd_pkt.payload));
    sender.snd_pkt.seqnum = sender.seqnum;
    sender.snd_pkt.checksum = 0;
    sender.snd_pkt.checksum = get_checksum(&sender.snd_pkt);
    tolayer3(A, sender.snd_pkt);

    starttimer(A, TMO_INCR);
    sender.state = SENDER_WAIT_ACK;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet) struct pkt packet;
{
    if (sender.state != SENDER_WAIT_ACK ||  //
        packet.acknum != sender.seqnum ||   //
        get_checksum(&packet) != 0)
        return;

    stoptimer(A);
    sender.seqnum ^= 1;
    sender.state = SENDER_WAIT_CALL;

    const struct msg *msg = msg_buf_pop();
    if (msg != NULL)
        A_output(*msg);
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    if (sender.state != SENDER_WAIT_ACK)
        return;

    tolayer3(A, sender.snd_pkt);
    starttimer(A, TMO_INCR);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    sender.state = SENDER_WAIT_CALL;
    sender.seqnum = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet) struct pkt packet;
{
    if (get_checksum(&packet) != 0)
        return;

    packet.acknum = packet.seqnum;
    packet.checksum = 0;
    packet.checksum = get_checksum(&packet);
    tolayer3(B, packet);

    if (packet.seqnum == receiver.seqnum) {
        tolayer5(B, packet.payload);
        receiver.seqnum ^= 1;
    }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    receiver.seqnum = 0;
}
