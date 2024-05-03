#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/verified/vigor-time.h"

#define DROP ((uint16_t)-1)
#define FLOOD ((uint16_t)-2)

struct nf_config;
struct rte_mbuf;

bool nf_init(void);
int nf_process(uint16_t device, uint8_t **buffer, uint16_t packet_length,
               time_ns_t now, struct rte_mbuf *mbuf);

extern struct nf_config config;
void nf_config_init(int argc, char **argv);
void nf_config_usage(void);
void nf_config_print(void);

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_border(unsigned lcore_id, time_ns_t time);
#endif
