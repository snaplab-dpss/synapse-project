#ifndef _BLOOM_FILTER_
#define _BLOOM_FILTER_

#include "../headers.p4"
#include "../constants.p4"

control BloomFilterRow(in hash_t hash, inout entry_t entry) {
    Register<entry_t, _>(WIDTH, 0) reg_bloom_filter_row;

    RegisterAction<entry_t, hash_t, entry_t>(reg_bloom_filter_row) read_update_action = {
        void apply(inout entry_t current, out entry_t out_value) {
            out_value = current;
            current = 1;
        }
    };

    action read_update() {
        entry_t tmp = read_update_action.execute(hash);
        entry = entry + tmp;
    }

    apply {
        read_update();
    }
}

control BloomFilter(inout ig_meta_t ig_md) {
    BloomFilterRow() bloom_filter_row0;
    BloomFilterRow() bloom_filter_row1;
    BloomFilterRow() bloom_filter_row2;

    apply {
        bloom_filter_row0.apply(ig_md.hash0, ig_md.entry);
        bloom_filter_row1.apply(ig_md.hash1, ig_md.entry);
        bloom_filter_row2.apply(ig_md.hash2, ig_md.entry);

        if (ig_md.entry == 3) {
            ig_md.found = 1;
        } else {
            ig_md.found = 0;
        }
    }
}

#endif /* _BLOOM_FILTER_ */