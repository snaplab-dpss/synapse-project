#ifndef _HASHING_
#define _HASHING_

#include "../headers.p4"
#include "../constants.p4"

control Hashing(
    inout header_t  hdr,
    inout ig_meta_t ig_md
) {
    Hash<hash_t>(HashAlgorithm_t.CRC32) hash0_calculator;
    Hash<hash_t>(HashAlgorithm_t.CRC32) hash1_calculator;
    Hash<hash_t>(HashAlgorithm_t.CRC32) hash2_calculator;

    action calc_hash0() {
        ig_md.hash0 = hash0_calculator.get({
            hdr.ipv4.src_addr,
            hdr.ipv4.dst_addr,
            HASH_SALT_0
        });
    }

    action calc_hash1() {
        ig_md.hash1 = hash1_calculator.get({
            hdr.ipv4.src_addr,
            hdr.ipv4.dst_addr,
            HASH_SALT_1
        });
    }

    action calc_hash2() {
        ig_md.hash2 = hash2_calculator.get({
            hdr.ipv4.src_addr,
            hdr.ipv4.dst_addr,
            HASH_SALT_2
        });
    }
    
    apply {
        calc_hash0();
        calc_hash1();
        calc_hash2();
    }
}

#endif /* _HASHING_ */