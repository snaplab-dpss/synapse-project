control CuckooHashTable(in bit<32> now, inout cuckoo_h cuckoo, out bool success) {
	Hash<bit</*@{CUCKOO_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_1;
	Hash<bit</*@{CUCKOO_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_2;
	Hash<bit</*@{CUCKOO_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) cuckoo_hash_func_2_r;

	bit</*@{CUCKOO_IDX_WIDTH}@*/> cuckoo_hash_1 = 0;
	bit</*@{CUCKOO_IDX_WIDTH}@*/> cuckoo_hash_2 = 0;
	bit</*@{CUCKOO_IDX_WIDTH}@*/> cuckoo_hash_2_r = 0;

	action calc_cuckoo_hash_1() { cuckoo_hash_1	= cuckoo_hash_func_1.get({HASH_SALT_1, cuckoo.key}); }
	action calc_cuckoo_hash_2() { cuckoo_hash_2	= cuckoo_hash_func_2.get({HASH_SALT_2, cuckoo.key}); }
	action calc_cuckoo_hash_2_r() { cuckoo_hash_2_r = cuckoo_hash_func_2_r.get({HASH_SALT_2, cuckoo.key}); }

	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_k_1;
	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_k_2;

	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_v_1;
	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_v_2;

	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_ts_1;
	Register<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>>(/*@{CUCKOO_ENTRIES}@*/, 0) reg_ts_2;

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bool>(reg_k_1) k_1_read = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bool>(reg_k_2) k_2_read = {
		void apply(inout bit<32> val, out bool match) {
			if (val == cuckoo.key) {
				match = true;
			} else {
				match = false;
			}
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_k_1) k_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_k_2) k_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.key;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_v_1) v_1_read_or_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			if (cuckoo.op == cuckoo_ops_t.UPDATE) {
				val = cuckoo.val;
			}
			res = val;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_v_2) v_2_read_or_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			if (cuckoo.op == cuckoo_ops_t.UPDATE) {
				val = cuckoo.val;
			}
			res = val;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_v_1) v_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_v_2) v_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.val;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bool>(reg_ts_1) ts_1_query_and_refresh = {
		void apply(inout bit<32> val, out bool active) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > /*@{ENTRY_TIMEOUT}@*/) {
				active = false;
				val = 0;
			} else {
				active = true;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bool>(reg_ts_2) ts_2_query_and_refresh = {
		void apply(inout bit<32> val, out bool active) {
			bit<32> diff = cuckoo.ts - val;
			if (diff > /*@{ENTRY_TIMEOUT}@*/) {
				active = false;
				val = 0;
			} else {
				active = true;
				val = cuckoo.ts;
			}
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_ts_1) ts_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.ts;
		}
	};

	RegisterAction<bit<32>, bit</*@{CUCKOO_IDX_WIDTH}@*/>, bit<32>>(reg_ts_2) ts_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = cuckoo.ts;
		}
	};

	action ts_diff(in bit<32> ts, out bit<32> diff) {
		diff = now - ts;
	}

	apply {
		cuckoo.old_op = cuckoo.op;
		cuckoo.old_key = cuckoo.key;

		calc_cuckoo_hash_1();
		calc_cuckoo_hash_2();
		calc_cuckoo_hash_2_r();

		success = false;
		if (cuckoo.op == cuckoo_ops_t.LOOKUP || cuckoo.op == cuckoo_ops_t.UPDATE) {
			if (k_1_read.execute(cuckoo_hash_1)) {
				if (ts_1_query_and_refresh.execute(cuckoo_hash_1)) {
					cuckoo.val = v_1_read_or_update.execute(cuckoo_hash_1);
					cuckoo.op = cuckoo_ops_t.DONE;
					success = true;
				}
			}
			if (!success) {
				if (k_2_read.execute(cuckoo_hash_2)) {
					if (ts_2_query_and_refresh.execute(cuckoo_hash_2)) {
						cuckoo.val = v_2_read_or_update.execute(cuckoo_hash_2);
						cuckoo.op = cuckoo_ops_t.DONE;
						success = true;
					}
				}
			}
		} else {
			cuckoo.key = k_1_swap.execute(cuckoo_hash_1);
			cuckoo.ts = ts_1_swap.execute(cuckoo_hash_1);
			cuckoo.val = v_1_swap.execute(cuckoo_hash_1);

			bit<32> ts_1_diff;
			ts_diff(cuckoo.ts, ts_1_diff);

			if (ts_1_diff < /*@{ENTRY_TIMEOUT}@*/) {
				cuckoo.key = k_2_swap.execute(cuckoo_hash_2_r);
				cuckoo.ts = ts_2_swap.execute(cuckoo_hash_2_r);
				cuckoo.val = v_2_swap.execute(cuckoo_hash_2_r);

				bit<32> ts_2_diff;
				ts_diff(cuckoo.ts, ts_2_diff);

				if (ts_2_diff < /*@{ENTRY_TIMEOUT}@*/) {
					cuckoo.op = cuckoo_ops_t.SWAP;
				} else {
					cuckoo.op = cuckoo_ops_t.DONE;
					success = true;
				}
			} else {
				cuckoo.op = cuckoo_ops_t.DONE;
				success = true;
			}
		}
	}
}

control CuckooHashBloomFilter(inout cuckoo_h cuckoo, out fwd_op_t fwd_op) {
	Hash<bit</*@{BLOOM_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) hash_old_key;
	Hash<bit</*@{BLOOM_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) hash_new_key;
	Hash<bit</*@{BLOOM_IDX_WIDTH}@*/>>(HashAlgorithm_t.CRC32) hash_old_key_2;

	Register<bit<16>, bit</*@{BLOOM_IDX_WIDTH}@*/>>(/*@{BLOOM_ENTRIES}@*/, 0) swap_transient;
	Register<bit<16>, bit</*@{BLOOM_IDX_WIDTH}@*/>>(/*@{BLOOM_ENTRIES}@*/, 0) swapped_transient;

	bit<16> swapped_transient_val = 0;

	RegisterAction<bit<16>, bit</*@{BLOOM_IDX_WIDTH}@*/>, bool>(swap_transient) swap_transient_read = {
		void apply(inout bit<16> val, out bool stable) {
			if (val <= swapped_transient_val) {
				stable = true;
			} else {
				stable = false;
			}
		}
	};

	RegisterAction<bit<16>, bit</*@{BLOOM_IDX_WIDTH}@*/>, bool>(swap_transient) swap_transient_conditional_inc = {
		void apply(inout bit<16> val, out bool new_insertion) {
			if (val <= swapped_transient_val) {
				val = swapped_transient_val |+| 1;
				new_insertion = true;
			} else {
				new_insertion = false;
			}
		}
	};

	RegisterAction<bit<16>, bit</*@{BLOOM_IDX_WIDTH}@*/>, bit<16>>(swap_transient) swap_transient_inc = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit</*@{BLOOM_IDX_WIDTH}@*/>, bit<16>>(swapped_transient) swapped_transient_inc = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit</*@{BLOOM_IDX_WIDTH}@*/>, bit<16>>(swapped_transient) swapped_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	apply {
		bit</*@{BLOOM_IDX_WIDTH}@*/> old_key_hash = hash_old_key.get({cuckoo.old_key});
		bit</*@{BLOOM_IDX_WIDTH}@*/> old_key_hash_2 = hash_old_key_2.get({cuckoo.old_key});

		if (cuckoo.op == cuckoo_ops_t.DONE) {
			if (cuckoo.old_op == cuckoo_ops_t.INSERT || cuckoo.old_op == cuckoo_ops_t.SWAP) {
				swapped_transient_inc.execute(old_key_hash);
			}
		} else if (cuckoo.op == cuckoo_ops_t.LOOKUP) {
			swapped_transient_val = swapped_transient_read.execute(old_key_hash);
			if (swap_transient_read.execute(old_key_hash)) {
				// Cache miss.
				cuckoo.op = cuckoo_ops_t.DONE;
			}
		} else if (cuckoo.op == cuckoo_ops_t.UPDATE) {
			if (cuckoo.recirc_cntr >= /*@{MAX_LOOPS}@*/) {
				// Give up and send to KVS server.
				swapped_transient_inc.execute(old_key_hash_2);
				cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				swapped_transient_val = swapped_transient_read.execute(old_key_hash_2);
				bool new_insertion = swap_transient_conditional_inc.execute(old_key_hash_2);
				if (new_insertion) {
					cuckoo.op = cuckoo_ops_t.INSERT;
					cuckoo.recirc_cntr = 0;
				}
			}
		} else if (cuckoo.op == cuckoo_ops_t.SWAP) {
			swapped_transient_inc.execute(old_key_hash_2);
			if (cuckoo.recirc_cntr >= /*@{MAX_LOOPS}@*/) {
				cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				swap_transient_inc.execute(hash_new_key.get({cuckoo.key}));
			}
		}

		if (cuckoo.op != cuckoo_ops_t.DONE) {
			if (cuckoo.recirc_cntr >= /*@{MAX_LOOPS}@*/) {
				cuckoo.op = cuckoo_ops_t.DONE;
			} else {
				fwd_op = fwd_op_t.RECIRCULATE;
				cuckoo.recirc_cntr = cuckoo.recirc_cntr + 1;
			}
		}
	}
}
