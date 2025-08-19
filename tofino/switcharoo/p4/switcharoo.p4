#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "includes/parser.p4"
#include "includes/deparser.p4"
#include "includes/headers.p4"
#include "includes/constants.p4"
#include "includes/debug.p4"

// ---------------------------------------------------------------------------
// Pipeline - Ingress
// ---------------------------------------------------------------------------

control Ingress(inout header_t hdr,
				inout ingress_metadata_t ig_md,
				in ingress_intrinsic_metadata_t ig_intr_md,
				in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
				inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
				inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_table_1_ts;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_table_2_ts;

	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swap_transient;
	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swapped_transient;

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1_k) read_1_k = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2_k) read_2_k = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1_k) swap_1_k = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = ig_md.cur_key;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2_k) swap_2_k = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = ig_md.table_1_key;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1_v) read_1_v = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2_v) read_2_v = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1_v) write_1_v = {
		void apply(inout bit<32> val) {
			val = ig_md.cur_val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2_v) write_2_v = {
		void apply(inout bit<32> val) {
			val = ig_md.cur_val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1_v) swap_1_v = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = ig_md.cur_val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2_v) swap_2_v = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val= ig_md.table_1_val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts) table_1_ts_query_and_refresh = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_md.entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
				val = 0;
			} else {
				res = val;
				val = ig_md.entry_ts;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts) table_2_ts_query_and_refresh = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_md.entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
				val = 0;
			} else {
				res = val;
				val = ig_md.entry_ts;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts) table_1_ts_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = ig_md.entry_ts;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts) table_2_ts_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = ig_md.entry_ts;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bool>(swap_transient) swap_transient_read = {
		void apply(inout bit<16> val, out bool stable) {
			if (val <= ig_md.swapped_transient_val) {
				stable = true;
			} else {
				stable = false;
			}
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bool>(swap_transient) swap_transient_conditional_inc = {
		void apply(inout bit<16> val, out bool new_insertion) {
			if (val <= ig_md.swapped_transient_val) {
				val = ig_md.swapped_transient_val |+| 1;
				new_insertion = true;
			} else {
				new_insertion = false;
			}
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_incr = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient) swapped_transient_incr = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient) swapped_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	action set_out_port(PortId_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action set_server_reply() {
		ig_md.is_server_reply = 1;
	}

	action set_not_server_reply() {
		ig_md.is_server_reply = 0;
	}

	table is_server_reply {
		key = {
			ig_intr_md.ingress_port : exact;
		}
		actions = {
			set_server_reply;
			set_not_server_reply;
		}

		const entries = {
			KVS_SERVER_PORT : set_server_reply();
		}

		const default_action = set_not_server_reply;
		size = 1;
	}

	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_table_1;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_table_2;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_table_2_r;

	action calc_hash_table_1()		{ ig_md.hash_table_1	= hash_table_1.get({ig_md.cur_key, HASH_SALT_1});	}
	action calc_hash_table_2()		{ ig_md.hash_table_2	= hash_table_2.get({ig_md.cur_key, HASH_SALT_2});	}
	action calc_hash_table_2_r()	{ ig_md.hash_table_2_r	= hash_table_2_r.get({ig_md.cur_key, HASH_SALT_2});	}

	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_old_key;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_new_key;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32) hash_new_key_2;

	table select_recirc_port {
		key = {
			ig_intr_md.ingress_port[8:7] : exact;
		}
		actions = {
			set_out_port;
		}
		size = 4;
		const entries = {
			0 : set_out_port(RECIRC_PORT_0);
			1 : set_out_port(RECIRC_PORT_1);
			2 : set_out_port(RECIRC_PORT_2);
			3 : set_out_port(RECIRC_PORT_3);
		}
	}

	action remove_extra_hdrs() {
		hdr.cuckoo.setInvalid();
	}

	table table_remove_extra_hdrs {
		key = {
			ig_tm_md.ucast_egress_port : exact;
		}

		actions = {
			remove_extra_hdrs;
			NoAction;
		}

		const entries = {
			RECIRC_PORT_0 : NoAction();
			RECIRC_PORT_1 : NoAction();
			RECIRC_PORT_2 : NoAction();
			RECIRC_PORT_3 : NoAction();
		}

		const default_action = remove_extra_hdrs;

		size = 8;
	}

	action set_client_port() {
		hdr.kv.port = (bit<16>)ig_intr_md.ingress_port;
	}

	table table_set_client_port {
		key = {
			ig_intr_md.ingress_port : exact;
		}

		actions = {
			set_client_port;
			NoAction;
		}

		const entries = {
			RECIRC_PORT_0 : NoAction();
			RECIRC_PORT_1 : NoAction();
			RECIRC_PORT_2 : NoAction();
			RECIRC_PORT_3 : NoAction();
		}

		const default_action = set_client_port;

		size = 8;
	}

	/********** DEBUG VARIABLES **********/
	DECLARE_DEBUG_VAR_WITH_INC(max_recirculations)
	DECLARE_DEBUG_VAR_WITH_INC(state_insert)
	DECLARE_DEBUG_VAR_WITH_INC(state_aborted_insert)
	DECLARE_DEBUG_VAR_WITH_INC(state_insert_from_max_loops)
	DECLARE_DEBUG_VAR_WITH_INC(state_nop)
	DECLARE_DEBUG_VAR_WITH_INC(state_swap)
	DECLARE_DEBUG_VAR_WITH_INC(state_swapped)
	DECLARE_DEBUG_VAR_WITH_INC(table1_insert)
	DECLARE_DEBUG_VAR_WITH_INC(table2_insert)
	DECLARE_DEBUG_VAR_WITH_INC(table1_evict)
	DECLARE_DEBUG_VAR_WITH_INC(table2_evict)
	DECLARE_DEBUG_VAR_WITH_INC(cache_hit)
	DECLARE_DEBUG_VAR_WITH_INC(expired_entry)
	DECLARE_DEBUG_VAR_WITH_INC(cache_miss_on_kvs_get)
	/*************************************/

	action ts_diff(in bit<32> ts, out bit<32> diff) {
		diff = ig_intr_md.ingress_mac_tstamp[47:16] - ts;
	}

	apply {
		if (hdr.cuckoo.isValid() && hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
			ig_md.cur_key		= hdr.cuckoo.key;
			ig_md.cur_val		= hdr.cuckoo.val;
			ig_md.entry_ts 		= hdr.cuckoo.ts;
		} else {
			ig_md.cur_key		= hdr.kv.key;
			ig_md.cur_val		= hdr.kv.val;
			ig_md.entry_ts 		= ig_intr_md.ingress_mac_tstamp[47:16];
		}

		is_server_reply.apply();

		calc_hash_table_1();

		if (ig_md.is_server_reply == 0) {
			if (!hdr.cuckoo.isValid()) {
				hdr.cuckoo.setValid();
				hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
				hdr.cuckoo.recirc_cntr = 0;
				hdr.kv.status = KVS_STATUS_CUCKOO;
			}

			if (hdr.cuckoo.op == cuckoo_ops_t.INSERT || hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
				if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
					ig_md.was_insert_op = true;
				}

				ig_md.table_1_key = swap_1_k.execute(ig_md.hash_table_1);

				bit<32> table_1_ts = table_1_ts_swap.execute(ig_md.hash_table_1);

				bit<32> table_1_ts_diff;
				ts_diff(table_1_ts, table_1_ts_diff);


				ig_md.table_1_val = swap_1_v.execute(ig_md.hash_table_1);

				// Compare the previously stored values against the current pkt's.
				// If the previous stored values haven't expired and they don't match
				// the current pkt's, we will swap them to Table 2.

				// The previous Table 1 entry was occupied and not yet expired,
				// so we'll swap it to Table 2.
				if (table_1_ts_diff < ENTRY_TIMEOUT) {
					debug_table1_evict_inc();

					calc_hash_table_2_r();
					
					key_t table_2_key = swap_2_k.execute(ig_md.hash_table_2_r);
					bit<32> table_2_ts = table_2_ts_swap.execute(ig_md.hash_table_2_r);

					bit<32> table_2_ts_diff;
					ts_diff(table_2_ts, table_2_ts_diff);

					val_t table_2_val = 0;
					table_2_val = swap_2_v.execute(ig_md.hash_table_2_r);

					// The previous Table 2 entry was occupied and not yet expired,
					// so we'll recirculate and swap it to Table 1.
					if (table_2_ts_diff < ENTRY_TIMEOUT) {
						debug_table2_evict_inc();

						ig_md.has_next_swap		= true;

						hdr.cuckoo.key	= table_2_key;
						hdr.cuckoo.val	= table_2_val;
						hdr.cuckoo.ts	= table_2_ts;

						if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
							// First (re)circulation for the current pkt.
							// Store the swap entry values and change the op to SWAP.
							// The packet will be recirculated later.
							hdr.cuckoo.op = cuckoo_ops_t.SWAP;
						} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
							// The current pkt is already a mirrored pkt.
							// Store the new swap entry values and change the op to SWAPPED.
							// The packet will be recirculated later.
							hdr.cuckoo.op = cuckoo_ops_t.SWAPPED;
						}
					} else {
						debug_table2_insert_inc();
						// The previous Table 2 entry was expired/replaced.
						// In its place is now the cur pkt.
						if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
							// Send the current pkt to the bloom filter with op == DONE, so it'll be sent out.
							hdr.cuckoo.op = cuckoo_ops_t.DONE;
						} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
							// Send the current pkt to the bloom filter with op == SWAPPED, to update the transient.
							hdr.cuckoo.op = cuckoo_ops_t.SWAPPED;
						}
					}
				} else {
					debug_table1_insert_inc();

					// The previous Table 1 entry was expired/replaced.
					// In its place is now the cur pkt.
					if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
						// Send the current pkt to the bloom filter with op == DONE, so it'll be sent out.
						hdr.cuckoo.op = cuckoo_ops_t.DONE;
					} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
						// Send the current pkt to the bloom filter with op == SWAPPED, to update the transient.
						hdr.cuckoo.op = cuckoo_ops_t.SWAPPED;
					}
				}
			} else {
				// Lookup packet

				table_set_client_port.apply();

				bit<32> table_1_key = read_1_k.execute(ig_md.hash_table_1);

				bool table_1_hit = false;

				if (table_1_key == ig_md.cur_key) {
					bit<32> stored_table_1_ts = table_1_ts_query_and_refresh.execute(ig_md.hash_table_1);

					if (stored_table_1_ts != 0) {
						if (hdr.kv.op == kv_ops_t.GET) {
							hdr.kv.val = read_1_v.execute(ig_md.hash_table_1);
						} else {
							write_1_v.execute(ig_md.hash_table_1);
						}

						hdr.cuckoo.op = cuckoo_ops_t.DONE;
						hdr.kv.status = KVS_STATUS_HIT;
						table_1_hit = true;
						debug_cache_hit_inc();
					} else {
						debug_expired_entry_inc();
					}
				}
				
				if (!table_1_hit) {
					calc_hash_table_2();

					bit<32> table_2_key = read_2_k.execute(ig_md.hash_table_2);

					bool table_2_hit = false;

					if (table_2_key == ig_md.cur_key) {
						bit<32> stored_table_2_ts = table_2_ts_query_and_refresh.execute(ig_md.hash_table_2);

						if (stored_table_2_ts != 0) {
							if (hdr.kv.op == kv_ops_t.GET) {
								hdr.kv.val = read_2_v.execute(ig_md.hash_table_2);
							} else {
								write_2_v.execute(ig_md.hash_table_2);
							}
							
							hdr.cuckoo.op = cuckoo_ops_t.DONE;
							hdr.kv.status = KVS_STATUS_HIT;
							table_2_hit = true;
							debug_cache_hit_inc();
						} else {
							debug_expired_entry_inc();
						}
					}
					
					if (!table_2_hit) {
						// Lookup failed, recirculate the packet to insert it in the table if it's a write packet.
						if (hdr.kv.op == kv_ops_t.PUT) {
							hdr.cuckoo.op = cuckoo_ops_t.INSERT;
						}
					}
				}
			}

			/**************************************************************
			*
			*                       Bloom filter logic
			* 
			***************************************************************/

			bit<BLOOM_IDX_WIDTH> old_key_hash = hash_old_key.get(ig_md.cur_key);
			bit<BLOOM_IDX_WIDTH> new_key_hash = hash_new_key.get(hdr.cuckoo.key);

			if (hdr.cuckoo.op == cuckoo_ops_t.DONE) {
				if (ig_md.was_insert_op) {
					swapped_transient_incr.execute(old_key_hash);
				}
			} else if (hdr.cuckoo.op == cuckoo_ops_t.LOOKUP) {
				ig_md.swapped_transient_val	= swapped_transient_read.execute(old_key_hash);
				if (swap_transient_read.execute(old_key_hash)) {
					ig_md.send_to_kvs_server = true;
					debug_cache_miss_on_kvs_get_inc();
				}
			} else if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
				debug_state_insert_inc();

				if (hdr.cuckoo.recirc_cntr >= MAX_LOOPS) {
					// Assume the insertion packet was lost.
					// Give up and send to the KVS server.
					ig_md.send_to_kvs_server = true;
					swapped_transient_incr.execute(old_key_hash);
					debug_state_insert_from_max_loops_inc();
				} else {
					ig_md.swapped_transient_val	= swapped_transient_read.execute(old_key_hash);
					bool new_insertion = swap_transient_conditional_inc.execute(old_key_hash);

					if (!new_insertion) {
						// However, if another pkt matching the same bloom idx is already recirculating, the current pkt will be sent back as a LOOKUP.
						hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
						debug_state_aborted_insert_inc();
					} else {
						hdr.cuckoo.recirc_cntr = 0;
					}
				}
			} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
				debug_state_swap_inc();
				swapped_transient_incr.execute(old_key_hash);
				swap_transient_incr.execute(hash_new_key_2.get(hdr.cuckoo.key));
			} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAPPED) {
				debug_state_swapped_inc();
				swapped_transient_incr.execute(old_key_hash);
				if (ig_md.has_next_swap) {
					swap_transient_incr.execute(hash_new_key_2.get(hdr.cuckoo.key));
					hdr.cuckoo.op = cuckoo_ops_t.SWAP;
				} else {
					hdr.cuckoo.op = cuckoo_ops_t.DONE;
				}
			}

			if (ig_md.send_to_kvs_server) {
				hdr.kv.key = hdr.cuckoo.key;
				hdr.kv.val = hdr.cuckoo.val;
				set_out_port(KVS_SERVER_PORT);
			} else if (hdr.cuckoo.op == cuckoo_ops_t.DONE) {
				debug_state_nop_inc();
				set_out_port((bit<9>)hdr.kv.port);
			} else if (hdr.cuckoo.recirc_cntr >= MAX_LOOPS) {
				hdr.kv.key = hdr.cuckoo.key;
				hdr.kv.val = hdr.cuckoo.val;
				set_out_port(KVS_SERVER_PORT);
				debug_max_recirculations_inc();
			} else {
				hdr.cuckoo.recirc_cntr = hdr.cuckoo.recirc_cntr + 1;
				select_recirc_port.apply();
			}
		} else {
			set_out_port((bit<9>)hdr.kv.port);
		}

		table_remove_extra_hdrs.apply();

		ig_tm_md.bypass_egress = 1;
	}
}

// ---------------------------------------------------------------------------
// Pipeline - Egress
// ---------------------------------------------------------------------------

control Egress(inout header_t hdr,
			   inout egress_metadata_t eg_md,
			   in egress_intrinsic_metadata_t eg_intr_md,
			   in egress_intrinsic_metadata_from_parser_t eg_prsr_md,
			   inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
			   inout egress_intrinsic_metadata_for_output_port_t eg_oport_md) {
	apply {}
}

// ---------------------------------------------------------------------------
// Instantiation
// ---------------------------------------------------------------------------

Pipeline(IngressParser(),
		 Ingress(),
		 IngressDeparser(),
		 EgressParser(),
		 Egress(),
		 EgressDeparser()) pipe;

Switch(pipe) main;
