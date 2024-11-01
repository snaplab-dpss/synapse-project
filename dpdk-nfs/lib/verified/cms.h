#ifndef _CMS_H_INCLUDED_
#define _CMS_H_INCLUDED_

#include "cms-util.h"

#include <stdbool.h>
#include <stdint.h>

#include "lib/verified/map.h"
#include "lib/verified/vigor-time.h"

struct CMS;

int cms_allocate(uint32_t capacity, uint16_t threshold, uint32_t key_size,
                 struct CMS **cms_out);
void cms_compute_hashes(struct CMS *cms, void *k);
void cms_refresh(struct CMS *cms, time_ns_t now);
int cms_fetch(struct CMS *cms);
int cms_touch_buckets(struct CMS *cms, time_ns_t now);
void cms_expire(struct CMS *cms, time_ns_t time);

#endif