#ifndef _HASHING_
#define _HASHING_

#include "cached_table.p4"
#include "../headers.p4"

control Hashing(in key_t k, out hash_t h) {
    Hash<hash_t>(HashAlgorithm_t.CRC32) hash_calculator;

    action calc_hash() {
        h = hash_calculator.get({k});
    }
    
    apply {
        calc_hash();
    }
}

#endif /* _HASHING_ */