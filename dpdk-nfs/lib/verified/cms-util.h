#ifndef _CMS_UTIL_H_INCLUDED_
#define _CMS_UTIL_H_INCLUDED_

// Careful: CMS_HASHES needs to be <= CMS_SALTS_BANK_SIZE
#define CMS_HASHES 4
#define CMS_SALTS_BANK_SIZE 64

#include <stdint.h>

extern const uint32_t CMS_SALTS[CMS_SALTS_BANK_SIZE];

#endif