#ifndef COLLECTION_H_
#define COLLECTION_H_

#include <stdint.h>

#include "simulator.h"

#define BUF_SIZE 1000

// #define RTT_DFT 30
// #define RTTEST_SHIFT 3
// #define RTTVAR_SHIFT 2

enum { A, B };

static struct {
    struct msg data[BUF_SIZE];
    int head, tail;
} msg_buf = {.head = 0, .tail = 0};

// static struct {
//     int est;
//     int var;
//     int send_time;
//     int cur_seq;
// } rtt_stats = {.est = 0, .var = 0, .send_time = 0, .cur_seq = -1};

static inline void msg_buf_push(const struct msg *msg)
{
    if ((msg_buf.tail + 1) % BUF_SIZE == msg_buf.head)
        return;
    msg_buf.data[msg_buf.tail] = *msg;
    msg_buf.tail = (msg_buf.tail + 1) % BUF_SIZE;
}

static inline const struct msg *msg_buf_pop()
{
    if (msg_buf.head == msg_buf.tail)
        return NULL;
    const struct msg *ret = &msg_buf.data[msg_buf.head];
    msg_buf.head = (msg_buf.head + 1) % BUF_SIZE;
    return ret;
}

// static int rto_update()
// {
//     int sample = get_sim_time() - rtt_stats.send_time;

//     if (rtt_stats.est == 0) {
//         rtt_stats.est = sample << RTTEST_SHIFT;
//         rtt_stats.var = sample << (RTTVAR_SHIFT - 1);
//     } else {
//         int delta = sample - (rtt_stats.est >> RTTEST_SHIFT);
//         if ((rtt_stats.est += delta) <= 0)
//             rtt_stats.est = 1;
//         if ((rtt_stats.var += (abs(delta) - (rtt_stats.var >> RTTVAR_SHIFT))) <= 0)
//             rtt_stats.var = 1;
//     }

//     return (rtt_stats.est >> RTTEST_SHIFT) + rtt_stats.var;
// }

static uint16_t get_checksum(const struct pkt *packet)
{
    uint32_t sum = 0xffff;
    const uint16_t *ptr = (const uint16_t *) packet;
    size_t cnt = sizeof(*packet) / sizeof(*ptr);

    while (cnt--)
        sum += *ptr++;

    while (sum >> 16)
        sum -= 0xffff;

    return ~sum;
}

#endif