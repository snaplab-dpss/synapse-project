#ifndef _HASHING_
#define _HASHING_

#include "cached_table.p4"

control Hashing(
	in  key_t  k,
	out hash_t hash
) {
	Hash<hash_t>(HashAlgorithm_t.CRC32) hash_calculator;

	action calc_hash() {
		hash = hash_calculator.get({
			k.src_addr,
			k.dst_addr,
			k.src_port,
			k.dst_port
		});
	}
	
	apply {
		calc_hash();
	}
}

#endif /* _HASHING_ */