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

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_k96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_k96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_1_v96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_2_v96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_table_1_ts;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES, 0) reg_table_2_ts;

	bit<32> table_1_ts_entry	= 0;
	bit<32> table_2_ts_entry	= 0;

	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swap_transient;
	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swapped_transient;

	#define KEY_READ(table, msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_##table##_k##lsb##_##msb) read_##table##_k##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
			} \
		};

	#define KEY_SWAP_1(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1##_k##lsb##_##msb) swap_1##_k##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val = ig_md.cur_key[msb:lsb]; \
			} \
		};

	#define KEY_SWAP_2(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2##_k##lsb##_##msb) swap_2##_k##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val = ig_md.table_1_key[msb:lsb]; \
			} \
		};

	#define VAL_READ(table, msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_##table##_v##lsb##_##msb) read_##table##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
			} \
		};

	#define VAL_WRITE(table, msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_##table##_v##lsb##_##msb) write_##table##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val) { \
				val = ig_md.cur_val[msb:lsb]; \
			} \
		};

	#define VAL_SWAP_1(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1##_v##lsb##_##msb) swap_1##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val = ig_md.cur_val[msb:lsb]; \
			} \
		};

	#define VAL_SWAP_2(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2##_v##lsb##_##msb) swap_2##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val= ig_md.table_1_val[msb:lsb]; \
			} \
		};

	KEY_READ(1, 31, 0)
	KEY_READ(1, 63, 32)
	KEY_READ(1, 95, 64)
	KEY_READ(1, 127, 96)

	KEY_READ(2, 31, 0)
	KEY_READ(2, 63, 32)
	KEY_READ(2, 95, 64)
	KEY_READ(2, 127, 96)

	KEY_SWAP_1(31, 0)
	KEY_SWAP_1(63, 32)
	KEY_SWAP_1(95, 64)
	KEY_SWAP_1(127, 96)

	KEY_SWAP_2(31, 0)
	KEY_SWAP_2(63, 32)
	KEY_SWAP_2(95, 64)
	KEY_SWAP_2(127, 96)

	VAL_READ(1, 31, 0)
	VAL_READ(1, 63, 32)
	VAL_READ(1, 95, 64)
	VAL_READ(1, 127, 96)

	VAL_READ(2, 31, 0)
	VAL_READ(2, 63, 32)
	VAL_READ(2, 95, 64)
	VAL_READ(2, 127, 96)

	VAL_WRITE(1, 31, 0)
	VAL_WRITE(1, 63, 32)
	VAL_WRITE(1, 95, 64)
	VAL_WRITE(1, 127, 96)

	VAL_WRITE(2, 31, 0)
	VAL_WRITE(2, 63, 32)
	VAL_WRITE(2, 95, 64)
	VAL_WRITE(2, 127, 96)

	VAL_SWAP_1(31, 0)
	VAL_SWAP_1(63, 32)
	VAL_SWAP_1(95, 64)
	VAL_SWAP_1(127, 96)

	VAL_SWAP_2(31, 0)
	VAL_SWAP_2(63, 32)
	VAL_SWAP_2(95, 64)
	VAL_SWAP_2(127, 96)

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts)
			table_1_ts_query = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_md.entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts)
			table_2_ts_query = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_md.entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts)
			table_1_ts_query_and_refresh = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_md.entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = ig_md.entry_ts;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts)
			table_2_ts_query_and_refresh = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_md.entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = ig_md.entry_ts;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts)
			table_1_ts_query_and_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_md.entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = ig_md.entry_ts;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts)
			table_2_ts_query_and_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_md.entry_ts - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = ig_md.entry_ts;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_incr = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bool>(swap_transient) swap_transient_conditional_inc = {
		void apply(inout bit<16> val, out bool new_insertion) {
			if (val == ig_md.swapped_transient_val) {
				val = val |+| 1;
				new_insertion = true;
			} else {
				new_insertion = false;
			}
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_clear = {
		void apply(inout bit<16> val) {
			val = 1;
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
	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient) swapped_transient_clear = {
		void apply(inout bit<16> val) {
			val = 0;
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

	CRCPolynomial<bit<16>>(
		coeff	 = 0x0589,
		reversed = false,
		msb		 = false,
		extended = false,
		init	 = 0x0000,
		xor		 = 0x0001
	) poly_crc_16_dect;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x0589,
		reversed = false,
		msb		 = false,
		extended = false,
		init	 = 0x0000,
		xor		 = 0x0001
	) poly_crc_16_dect2;

	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC16)  hash_table_1;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_dect) hash_table_2;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_dect2) hash_table_2_recirc;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello2;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello3;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello4;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello5;

	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello)	hash_swap;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello2) hash_swap_2;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello3) hash_swapped;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello4) hash_swapped_2;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello5) hash_swapped_3;

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
		hdr.cur_swap.setInvalid();
	}

	table remove_extra_hdrs_tbl {
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

		size = 4;
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
	// DECLARE_DEBUG_VAR_WITH_SET(time_now)
	// DECLARE_DEBUG_VAR_WITH_SET(time_swap)
	/*************************************/

	action set_from_cur_swap() {
		ig_md.cur_key		= hdr.cur_swap.key;
		ig_md.cur_val		= hdr.cur_swap.val;
		ig_md.entry_ts		= hdr.cur_swap.ts;
	}

	action set_from_not_cur_swap() {
		ig_md.cur_key		= hdr.kv.key;
		ig_md.cur_val		= hdr.kv.val;
		ig_md.entry_ts		= ig_prsr_md.global_tstamp[47:16];
	}

	table set_keys_values_ts {
		key = {
			hdr.cur_swap.isValid(): exact;
		}

		actions = {
			set_from_cur_swap;
			set_from_not_cur_swap;
		}

		const entries = {
			true	: set_from_cur_swap();
			false	: set_from_not_cur_swap();
		}

		size = 2;
	}

	apply {
		// if (hdr.cur_swap.isValid()) {
		// 	ig_md.cur_key		= hdr.cur_swap.key;
		// 	ig_md.cur_val		= hdr.cur_swap.val;
		// 	ig_md.entry_ts		= hdr.cur_swap.ts;
		// 	debug_time_swap_set(hdr.cur_swap.ts);
		// } else {
		// 	ig_md.cur_key		= hdr.kv.key;
		// 	ig_md.cur_val		= hdr.kv.val;
		// 	ig_md.entry_ts		= ig_prsr_md.global_tstamp[47:16];
		// 	debug_time_now_set(ig_prsr_md.global_tstamp[47:16]);
		// }

		set_keys_values_ts.apply();

		is_server_reply.apply();

		ig_md.hash_table_1 = hash_table_1.get({ig_md.cur_key});

		bool debug_table1_insert = false;
		bool debug_table1_evict = false;
		bool debug_table2_insert = false;
		bool debug_table2_evict = false;

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

				// Insert the value in Table 1.
				// If the entry is not empty, swap the old value in Table 2.

				// Insert the current pkt in table 1.
				// Get the previously values.
				bit<32> table_1_ts = table_1_ts_query_and_swap.execute(ig_md.hash_table_1);

				ig_md.table_1_key[31:0]		= swap_1_k0_31.execute(ig_md.hash_table_1);
				ig_md.table_1_key[63:32]	= swap_1_k32_63.execute(ig_md.hash_table_1);
				ig_md.table_1_key[95:64]	= swap_1_k64_95.execute(ig_md.hash_table_1);
				ig_md.table_1_key[127:96]	= swap_1_k96_127.execute(ig_md.hash_table_1);

				ig_md.table_1_val[31:0]		= swap_1_v0_31.execute(ig_md.hash_table_1);
				ig_md.table_1_val[63:32]	= swap_1_v32_63.execute(ig_md.hash_table_1);
				ig_md.table_1_val[95:64]	= swap_1_v64_95.execute(ig_md.hash_table_1);
				ig_md.table_1_val[127:96]	= swap_1_v96_127.execute(ig_md.hash_table_1);

				// Compare the previously stored values against the current pkt's.
				// If the previous stored values haven't expired and they don't match
				// the current pkt's, we will swap them to Table 2.

				// The previous Table 1 entry was occupied and not yet expired,
				// so we'll swap it to Table 2.
				if (table_1_ts_entry != 0) {
					debug_table1_evict = true;

					ig_md.hash_table_2_r = hash_table_2_recirc.get({ig_md.cur_key});
					
					bit<32> table_2_ts = table_2_ts_query_and_swap.execute(ig_md.hash_table_2_r);

					ig_md.table_2_key[31:0]		= swap_2_k0_31.execute(ig_md.hash_table_2_r);
					ig_md.table_2_key[63:32]	= swap_2_k32_63.execute(ig_md.hash_table_2_r);
					ig_md.table_2_key[95:64]	= swap_2_k64_95.execute(ig_md.hash_table_2_r);
					ig_md.table_2_key[127:96]	= swap_2_k96_127.execute(ig_md.hash_table_2_r);

					ig_md.table_2_val[31:0]		= swap_2_v0_31.execute(ig_md.hash_table_2_r);
					ig_md.table_2_val[63:32]	= swap_2_v32_63.execute(ig_md.hash_table_2_r);
					ig_md.table_2_val[95:64]	= swap_2_v64_95.execute(ig_md.hash_table_2_r);
					ig_md.table_2_val[127:96]	= swap_2_v96_127.execute(ig_md.hash_table_2_r);

					// The previous Table 2 entry was occupied and not yet expired,
					// so we'll recirculate and swap it to Table 1.
					if (table_2_ts_entry != 0) {
						debug_table2_evict = true;

						// Cur pkt has been sucessfully inserted.
						// The swap entry will be insert on hdr.next_swap.
						// During the bloom processing, hdr.next_swap will become hdr.cur_swap.
						ig_md.has_next_swap	= 1;
						hdr.cur_swap.setValid();
						hdr.cur_swap.key	= ig_md.table_2_key;
						hdr.cur_swap.val	= ig_md.table_2_val;
						hdr.cur_swap.ts		= table_2_ts;
						if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
							// First (re)circulation for the current pkt.
							// Store the swap entry values and change the op to SWAP.
							// The packet will be recirculated later.
							hdr.cuckoo.op = cuckoo_ops_t.SWAP;
						} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
							// The current pkt is already a mirrored pkt.
							// Store the new swap entry values and change the op to SWAPPED.
							// The packet will be recirculated later.
							hdr.cuckoo.op		= cuckoo_ops_t.SWAPPED;
							ig_md.swapped_key	= hdr.cur_swap.key;
						}
					} else {
						debug_table2_insert = true;
						// The previous Table 2 entry was expired/replaced.
						// In its place is now the cur pkt.
						if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
							// Send the current pkt to the bloom filter with op == NOP,
							// so it'll be sent out.
							hdr.cuckoo.op = cuckoo_ops_t.NOP;
						} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
							// Send the current pkt to the bloom filter with op == SWAPPED,
							// to update the transient.
							hdr.cuckoo.op = cuckoo_ops_t.SWAPPED;
						}
					}
				} else {
					debug_table1_insert = true;
					// The previous Table 1 entry was expired/replaced.
					// In its place is now the cur pkt.
					if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
						// Send the current pkt to the bloom filter with op == NOP,
						// so it'll be sent out.
						hdr.cuckoo.op = cuckoo_ops_t.NOP;
					} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
						// Send the current pkt to the bloom filter with op == SWAPPED,
						// to update the transient.
						hdr.cuckoo.op = cuckoo_ops_t.SWAPPED;
					}
				}
			} else {
				// Lookup packet

				hdr.kv.port = (bit<16>)ig_intr_md.ingress_port;

				bool table_1_hit = false;
				bool table_2_hit = false;

				bit<32> stored_table_1_ts = 0;
				if (hdr.kv.op == kv_ops_t.GET) {
					stored_table_1_ts = table_1_ts_query_and_refresh.execute(ig_md.hash_table_1);
				} else {
					stored_table_1_ts = table_1_ts_query.execute(ig_md.hash_table_1);
				}

				if (stored_table_1_ts != 0) {
					bit<32> table_1_key_0 = read_1_k0_31.execute(ig_md.hash_table_1);
					bit<32> table_1_key_1 = read_1_k32_63.execute(ig_md.hash_table_1);
					bit<32> table_1_key_2 = read_1_k64_95.execute(ig_md.hash_table_1);
					bit<32> table_1_key_3 = read_1_k96_127.execute(ig_md.hash_table_1);

					bool key_match = false;
					if (table_1_key_0 == ig_md.cur_key[31:0]) {
						if (table_1_key_1 == ig_md.cur_key[63:32]) {
							if (table_1_key_2 == ig_md.cur_key[95:64]) {
								if (table_1_key_3 == ig_md.cur_key[127:96]) {
									key_match = true;
								}
							}
						}
					}

					if (key_match) {
						if (hdr.kv.op == kv_ops_t.GET) {
							hdr.kv.val[31:0]	= read_1_v0_31.execute(ig_md.hash_table_1);
							hdr.kv.val[63:32]	= read_1_v32_63.execute(ig_md.hash_table_1);
							hdr.kv.val[95:64]	= read_1_v64_95.execute(ig_md.hash_table_1);
							hdr.kv.val[127:96]	= read_1_v96_127.execute(ig_md.hash_table_1);
						} else {
							write_1_v0_31.execute(ig_md.hash_table_1);
							write_1_v32_63.execute(ig_md.hash_table_1);
							write_1_v64_95.execute(ig_md.hash_table_1);
							write_1_v96_127.execute(ig_md.hash_table_1);
						}

						hdr.cuckoo.op = cuckoo_ops_t.NOP;
						hdr.kv.status = KVS_STATUS_HIT;
						table_1_hit = true;
					}
				}
				
				if (!table_1_hit) {
					ig_md.hash_table_2 = hash_table_2.get({ig_md.cur_key});

					bit<32> stored_table_2_ts = 0;
					if (hdr.kv.op == kv_ops_t.GET) {
						stored_table_2_ts = table_2_ts_query_and_refresh.execute(ig_md.hash_table_2);
					} else {
						stored_table_2_ts = table_2_ts_query.execute(ig_md.hash_table_2);
					}

					if (stored_table_2_ts != 0) {
						bit<32> table_2_key_0 = read_2_k0_31.execute(ig_md.hash_table_2);
						bit<32> table_2_key_1 = read_2_k32_63.execute(ig_md.hash_table_2);
						bit<32> table_2_key_2 = read_2_k64_95.execute(ig_md.hash_table_2);
						bit<32> table_2_key_3 = read_2_k96_127.execute(ig_md.hash_table_2);

						bool key_match = false;
						if (table_2_key_0 == ig_md.cur_key[31:0]) {
							if (table_2_key_1 == ig_md.cur_key[63:32]) {
								if (table_2_key_2 == ig_md.cur_key[95:64]) {
									if (table_2_key_3 == ig_md.cur_key[127:96]) {
										key_match = true;
									}
								}
							}
						}

						if (key_match) {
							if (hdr.kv.op == kv_ops_t.GET) {
								hdr.kv.val[31:0]	= read_2_v0_31.execute(ig_md.hash_table_2);
								hdr.kv.val[63:32]	= read_2_v32_63.execute(ig_md.hash_table_2);
								hdr.kv.val[95:64]	= read_2_v64_95.execute(ig_md.hash_table_2);
								hdr.kv.val[127:96]	= read_2_v96_127.execute(ig_md.hash_table_2);
							} else {
								write_2_v0_31.execute(ig_md.hash_table_2);
								write_2_v32_63.execute(ig_md.hash_table_2);
								write_2_v64_95.execute(ig_md.hash_table_2);
								write_2_v96_127.execute(ig_md.hash_table_2);
							}
							
							hdr.cuckoo.op = cuckoo_ops_t.NOP;
							hdr.kv.status = KVS_STATUS_HIT;
							table_2_hit = true;
						}
					}
					
					if (!table_2_hit) {
						// Lookup failed, recirculate the packet to insert it in the table if it's a write packet.
						if (hdr.kv.op == kv_ops_t.GET) {
							ig_md.send_to_kvs_server = true;
						} else {
							hdr.cuckoo.op = cuckoo_ops_t.INSERT;
						}
					}
				}
			}

			if (debug_table1_insert) {
				debug_table1_insert_inc();
			}
			if (debug_table1_evict) {
				debug_table1_evict_inc();
			}
			if (debug_table2_insert) {
				debug_table2_insert_inc();
			}
			if (debug_table2_evict) {
				debug_table2_evict_inc();
			}

			/**************************************************************
			*
			*                       Bloom filter logic
			* 
			***************************************************************/

			if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
				debug_state_insert_inc();
				bit<BLOOM_IDX_WIDTH> swap_transient_idx = hash_swap.get({ig_md.cur_key});

				// If we've reached MAX_LOOPS_INSERT recirculations,
				// then assume that the previous swap pkt is lost and reset the transients.
				if (hdr.cuckoo.recirc_cntr == MAX_LOOPS) {
					swap_transient_clear.execute(swap_transient_idx);
					swapped_transient_clear.execute(swap_transient_idx);
					hdr.cuckoo.recirc_cntr = 0;
					debug_state_insert_from_max_loops_inc();
				} else {
					ig_md.swapped_transient_val	= swapped_transient_read.execute(swap_transient_idx);
					bool new_insertion			= swap_transient_conditional_inc.execute(swap_transient_idx);

					if (!new_insertion) {
						// However, if another pkt matching the same bloom idx is already
						// recirculating, the current pkt will be sent back as a LOOKUP.
						hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
						debug_state_aborted_insert_inc();
					}
				}
			} else if (hdr.cuckoo.op == cuckoo_ops_t.NOP && ig_md.was_insert_op) {
				swapped_transient_incr.execute(hash_swapped.get({ig_md.cur_key}));
			} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
				debug_state_swap_inc();
				swap_transient_incr.execute(hash_swap_2.get({hdr.cur_swap.key}));
				swapped_transient_incr.execute(hash_swapped.get({ig_md.cur_key}));
			} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAPPED) {
				debug_state_swapped_inc();
				if (ig_md.has_next_swap == 0) {
					swapped_transient_incr.execute(hash_swapped_2.get({hdr.cur_swap.key}));
					hdr.cuckoo.op = cuckoo_ops_t.NOP;
				} else {
					swap_transient_incr.execute(hash_swap_2.get({hdr.cur_swap.key}));
					swapped_transient_incr.execute(hash_swapped_3.get({ig_md.swapped_key}));
					hdr.cuckoo.op = cuckoo_ops_t.SWAP;
				}
			}

			if (ig_md.send_to_kvs_server) {
				set_out_port(KVS_SERVER_PORT);
			} else if (hdr.cuckoo.op == cuckoo_ops_t.NOP) {
				debug_state_nop_inc();
				set_out_port((bit<9>)hdr.kv.port);
			} else if (hdr.cuckoo.recirc_cntr >= MAX_LOOPS) {
				set_out_port(KVS_SERVER_PORT);
				debug_max_recirculations_inc();
			} else {
				hdr.cuckoo.recirc_cntr = hdr.cuckoo.recirc_cntr + 1;
				select_recirc_port.apply();
			}
		} else {
			set_out_port((bit<9>)hdr.kv.port);
		}

		remove_extra_hdrs_tbl.apply();

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
