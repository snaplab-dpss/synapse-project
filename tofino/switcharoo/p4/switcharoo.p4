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

// ---------------------------------------------------------------------------
// Pipeline - Ingress
// ---------------------------------------------------------------------------

control Ingress(inout header_t hdr,
				inout ingress_metadata_t ig_md,
				in ingress_intrinsic_metadata_t ig_intr_md,
				in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
				inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
				inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

	/* Cuckoo declarations */

	Register<bit<32>, _>(1) cuckoo_recirc_pkts;
	Register<bit<32>, _>(1) ins_cntr;
	Register<bit<32>, _>(1) expired_entry_table_2;
	Register<bit<32>, _>(1) table_2_match_cntr;
	Register<bit<32>, _>(1) table_1_match_cntr;

	Register<bit<32>, _>(TABLE_SIZE) table_1_1;
	Register<bit<32>, _>(TABLE_SIZE) table_1_2;
	Register<bit<32>, _>(TABLE_SIZE) table_1_3;
	Register<bit<16>, _>(TABLE_SIZE) table_1_4;
	Register<bit<32>, _>(TABLE_SIZE) table_1_5;
	Register<bit<16>, _>(TABLE_SIZE) table_1_6;

	bit<32> table_1_1_entry = 0;
	bit<32> table_1_2_entry = 0;
	bit<32> table_1_3_entry = 0;
	bit<16> table_1_4_entry = 0;
	bit<32> table_1_5_entry = 0;
	bit<16> table_1_6_entry = 0;

	Register<bit<32>, _>(TABLE_SIZE) table_2_1;
	Register<bit<32>, _>(TABLE_SIZE) table_2_2;
	Register<bit<32>, _>(TABLE_SIZE) table_2_3;
	Register<bit<16>, _>(TABLE_SIZE) table_2_4;
	Register<bit<32>, _>(TABLE_SIZE) table_2_5;
	Register<bit<16>, _>(TABLE_SIZE) table_2_6;

	bit<32> key_1		= 0;
	bit<32> key_2		= 0;
	bit<32> ports		= 0;
	bit<16> ip_flow_id	= 0;
	bit<32> entry_ts	= 0;
	bit<16> entry_ts_2	= 0;

	Register<bit<16>, _>(SWAP_BLOOM_FILTER_SIZE) swap_transient;
	Register<bit<16>, _>(SWAP_BLOOM_FILTER_SIZE) swapped_transient;

	/* CUCKOO: TABLE 1 */
	/* Lookup Actions */

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bool>(table_1_1) table_1_1_lookup_action = {
		void apply(inout bit<32> val, out bool res) {
			if (hdr.ipv4.src_addr == val) {
				res = true;
			} else {
				res = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bool>(table_1_2) table_1_2_lookup_action = {
		void apply(inout bit<32> val, out bool res) {
			if (hdr.ipv4.dst_addr == val) {
				res = true;
			} else {
				res = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_1_3) table_1_3_read = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_HASH_BITS>, bit<16>>(table_1_4) table_1_4_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_1_5) table_1_5_write = {
		void apply(inout bit<32> val) {
			val = ig_prsr_md.global_tstamp[31:0];
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_HASH_BITS>, bit<16>>(table_1_6) table_1_6_write = {
		void apply(inout bit<16> val) {
			val = ig_prsr_md.global_tstamp[47:32];
		}
	};

	/* Swap Actions */

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_1_1) table_1_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = key_1;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_1_2) table_1_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = key_2;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_1_3) table_1_3_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = ports;
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_HASH_BITS>, bit<16>>(table_1_4) table_1_4_swap = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
			val = ip_flow_id;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_1_5) table_1_5_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_prsr_md.global_tstamp[31:0] - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = entry_ts;
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_HASH_BITS>, bit<16>>(table_1_6) table_1_6_swap = {
		void apply(inout bit<16> val, out bit<16> res) {
			bit<16> diff = ig_prsr_md.global_tstamp[47:32] - val;
			if (diff == 0) {
				res = val;
			} else {
				res = 0;
			}
			val = entry_ts_2;
		}
	};

	/* CUCKOO: TABLE 2 */
	/* Lookup Actions */

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bool>(table_2_1) table_2_1_lookup = {
		void apply(inout bit<32> val, out bool res) {
			if (hdr.ipv4.src_addr == val) {
				res = true;
			} else {
				res = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bool>(table_2_2) table_2_2_lookup = {
		void apply(inout bit<32> val, out bool res) {
			if (hdr.ipv4.dst_addr == val) {
				res = true;
			} else {
				res = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_2_3) table_2_3_read = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_HASH_BITS>, bit<16>>(table_2_4) table_2_4_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_2_5) table_2_5_write = {
		void apply(inout bit<32> val) {
			val = ig_prsr_md.global_tstamp[31:0];
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_HASH_BITS>, bit<16>>(table_2_6) table_2_6_write = {
		void apply(inout bit<16> val) {
			val = ig_prsr_md.global_tstamp[47:32];
		}
	};

	/* Swap Actions */

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_2_1) table_2_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = table_1_1_entry;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_2_2) table_2_2_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = table_1_2_entry;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_2_3) table_2_3_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			val = table_1_3_entry;
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_HASH_BITS>, bit<16>>(table_2_4) table_2_4_swap = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
			val = table_1_4_entry;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bit<32>>(table_2_5) table_2_5_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_prsr_md.global_tstamp[31:0] - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = table_1_5_entry;
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_HASH_BITS>, bit<16>>(table_2_6) table_2_6_swap = {
		void apply(inout bit<16> val, out bit<16> res) {
			bit<16> diff = ig_prsr_md.global_tstamp[47:32] - val;
			if (diff == 0) {
				res = val;
			} else {
				res = 0;
			}
			val = table_1_6_entry;
		}
	};

	/* CUCKOO: STATS REGISTERS */

	RegisterAction<bit<32>, bit<1>, bit<32>>(cuckoo_recirc_pkts) cuckoo_recirc_pkts_incr = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(ins_cntr) ins_cntr_incr = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(table_2_match_cntr) table_2_match_cntr_incr = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(table_1_match_cntr) table_1_match_cntr_incr = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	/* BLOOM: SWAP TRANSIENT */
	/* Lookup Actions */

	RegisterAction<bit<16>, bit<SWAP_BLOOM_FILTER_HASH_BITS>, bit<16>>(swap_transient)
			swap_transient_incr = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<SWAP_BLOOM_FILTER_HASH_BITS>, bit<16>>(swap_transient)
			swap_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	RegisterAction<bit<16>, bit<SWAP_BLOOM_FILTER_HASH_BITS>, bit<16>>(swap_transient)
			swap_transient_clear = {
		void apply(inout bit<16> val) {
			val = 0;
		}
	};

	/* BLOOM: SWAPPED TRANSIENT */
	/* Lookup Actions */

	RegisterAction<bit<16>, bit<SWAP_BLOOM_FILTER_HASH_BITS>, bit<16>>(swapped_transient)
			swapped_transient_incr = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<SWAP_BLOOM_FILTER_HASH_BITS>, bit<16>>(swapped_transient)
			swapped_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};
	RegisterAction<bit<16>, bit<SWAP_BLOOM_FILTER_HASH_BITS>, bit<16>>(swapped_transient)
			swapped_transient_clear = {
		void apply(inout bit<16> val) {
			val = 0;
		}
	};

	/* BLOOM: Forward*/

	action send(PortId_t port_number) {
		ig_tm_md.ucast_egress_port	= port_number;
		ig_tm_md.bypass_egress		= 0x1;
		ig_dprsr_md.drop_ctl		= 0x0;
	}

	@ternary(1)
	table fwd {
		key = {
			hdr.ipv4.identification: exact;
		}
		actions = {
			send;
		}
		size = 1024;
	}

	/* CUCKOO: Hash Definitions */

	Hash<bit<CUCKOO_HASH_BITS>>(HashAlgorithm_t.CRC16) hash_table_1;

	CRCPolynomial<bit<CUCKOO_HASH_BITS>>(
		coeff	 = 0x0589,
		reversed = false,
		msb		 = false,
		extended = false,
		init	 = 0x0000,
		xor		 = 0x0001
	) poly_crc_16_dect;

	CRCPolynomial<bit<CUCKOO_HASH_BITS>>(
		coeff	 = 0x0589,
		reversed = false,
		msb		 = false,
		extended = false,
		init	 = 0x0000,
		xor		 = 0x0001
	) poly_crc_16_dect2;

	Hash<bit<CUCKOO_HASH_BITS>>(HashAlgorithm_t.CUSTOM, poly_crc_16_dect)	hash_table_2;
	Hash<bit<CUCKOO_HASH_BITS>>(HashAlgorithm_t.CUSTOM, poly_crc_16_dect2)	hash_table_2_recirc;

	/* BLOOM: Hash Definitions */

	CRCPolynomial<bit<SWAP_BLOOM_FILTER_HASH_BITS>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello;

	CRCPolynomial<bit<SWAP_BLOOM_FILTER_HASH_BITS>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello2;

	CRCPolynomial<bit<SWAP_BLOOM_FILTER_HASH_BITS>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello3;

	CRCPolynomial<bit<SWAP_BLOOM_FILTER_HASH_BITS>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello4;

	Hash<bit<SWAP_BLOOM_FILTER_HASH_BITS>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello) hash_swap;
	Hash<bit<SWAP_BLOOM_FILTER_HASH_BITS>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello2)
			hash_swap_2;
	Hash<bit<SWAP_BLOOM_FILTER_HASH_BITS>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello3)
			hash_swapped;
	Hash<bit<SWAP_BLOOM_FILTER_HASH_BITS>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello4)
			hash_swapped_2;

	/* Logic to get a flow ID */

	Random<bit<2>>() flow_id_random_gen;
	bit<2> select_flow_id_index = flow_id_random_gen.get();
	action assign_ipv4_identification(bit<16> flow_id) {
		ip_flow_id = flow_id;
	}

	action recirculate_insert_ip1_to_cuckoo() {
		ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_INSERT_IP1_TO_CUCKOO;
		ig_tm_md.bypass_egress = 0x1;
	}

	action recirculate_insert_ip2_to_cuckoo() {
		ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_INSERT_IP2_TO_CUCKOO;
		ig_tm_md.bypass_egress = 0x1;
	}

	action recirculate_insert_ip3_to_cuckoo() {
		ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_INSERT_IP3_TO_CUCKOO;
		ig_tm_md.bypass_egress = 0x1;
	}

	action recirculate_insert_ip4_to_cuckoo() {
		ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_INSERT_IP4_TO_CUCKOO;
		ig_tm_md.bypass_egress = 0x1;
	}

	action recirculate_lookup_ip1_to_cuckoo() {
		ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_LOOKUP_IP1_TO_CUCKOO;
		ig_tm_md.bypass_egress = 0x1;
	}

	action recirculate_lookup_ip2_to_cuckoo() {
		ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_LOOKUP_IP2_TO_CUCKOO;
		ig_tm_md.bypass_egress = 0x1;
	}

	action recirculate_lookup_ip3_to_cuckoo() {
		ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_LOOKUP_IP3_TO_CUCKOO;
		ig_tm_md.bypass_egress = 0x1;
	}

	action recirculate_lookup_ip4_to_cuckoo() {
		ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_LOOKUP_IP4_TO_CUCKOO;
		ig_tm_md.bypass_egress = 0x1;
	}

	@ternary(1)
	table select_flow_id {
		key = {
			select_flow_id_index: exact;
		}
		actions = {
			assign_ipv4_identification;
		}
		size = 4;
		const entries = {
			0: assign_ipv4_identification(0xFFFF);
			1: assign_ipv4_identification(0xEEEE);
			2: assign_ipv4_identification(0xDDDD);
			3: assign_ipv4_identification(0xCCCC);
		}
	}

	/* Cuckoo Port Selection */

	bit<9> original_ingress_port = hdr.ethernet.dst_addr[24:16];

	@ternary(1)
	table select_cuckoo_recirc_port {
		key = {
			hdr.cuckoo_op.op		: exact;
			original_ingress_port	: exact;
		}
		actions = {
			recirculate_insert_ip1_to_cuckoo;
			recirculate_insert_ip2_to_cuckoo;
			recirculate_insert_ip3_to_cuckoo;
			recirculate_insert_ip4_to_cuckoo;
			recirculate_lookup_ip1_to_cuckoo;
			recirculate_lookup_ip2_to_cuckoo;
			recirculate_lookup_ip3_to_cuckoo;
			recirculate_lookup_ip4_to_cuckoo;
		}
		size = 8;
		const entries = {
			(cuckoo_ops_t.INSERT, INGRESS_PORT_1): recirculate_insert_ip1_to_cuckoo();
			(cuckoo_ops_t.INSERT, INGRESS_PORT_2): recirculate_insert_ip2_to_cuckoo();
			(cuckoo_ops_t.INSERT, INGRESS_PORT_3): recirculate_insert_ip3_to_cuckoo();
			(cuckoo_ops_t.INSERT, INGRESS_PORT_4): recirculate_insert_ip4_to_cuckoo();
			(cuckoo_ops_t.LOOKUP, INGRESS_PORT_1): recirculate_lookup_ip1_to_cuckoo();
			(cuckoo_ops_t.LOOKUP, INGRESS_PORT_2): recirculate_lookup_ip2_to_cuckoo();
			(cuckoo_ops_t.LOOKUP, INGRESS_PORT_3): recirculate_lookup_ip3_to_cuckoo();
			(cuckoo_ops_t.LOOKUP, INGRESS_PORT_4): recirculate_lookup_ip4_to_cuckoo();
		}
	}

	apply {
		if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAP) {
			// If swap pkt, get data from the swap header.
			key_1		= hdr.swap_entry.ip_src_addr;
			key_2		= hdr.swap_entry.ip_dst_addr;
			ports		= hdr.swap_entry.ports;
			ip_flow_id	= hdr.swap_entry.entry_value;
			entry_ts	= hdr.swap_entry.ts;
			entry_ts_2	= hdr.swap_entry.ts_2;
		} else {
			// Else, get data from the original headers.
			key_1		= hdr.ipv4.src_addr;
			key_2		= hdr.ipv4.dst_addr;
			ports		= ig_md.l4_lookup.src_port ++ ig_md.l4_lookup.dst_port;
			entry_ts	= ig_prsr_md.global_tstamp[31:0];
			entry_ts_2	= ig_prsr_md.global_tstamp[47:32];
		}

		bit<CUCKOO_HASH_BITS> idx_table_1 = hash_table_1.get({key_1,
															  key_2,
															  ports});

		if (hdr.cuckoo_op.isValid() && (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT ||
										hdr.cuckoo_op.op == cuckoo_ops_t.SWAP)) {
			// Insert the value in Table 1.
			// If the entry is not empty, swap the old value in Table 2.
			cuckoo_recirc_pkts_incr.execute(0);

			if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
				// If op == INSERT, then it is the first (re)circulation of this pkt.
				// Compute a new flow ID to serve as the k/v value.
				select_flow_id.apply();

				ins_cntr_incr.execute(0);
			}

			bool to_swap_1 = false;

			// Insert the current pkt in table 1.
			// Get the previously values.
			table_1_1_entry = table_1_1_swap.execute(idx_table_1);
			table_1_2_entry = table_1_2_swap.execute(idx_table_1);
			table_1_3_entry = table_1_3_swap.execute(idx_table_1);
			table_1_4_entry = table_1_4_swap.execute(idx_table_1);
			table_1_5_entry = table_1_5_swap.execute(idx_table_1);
			table_1_6_entry = table_1_6_swap.execute(idx_table_1);

			// Compare the previously stored values against the current pkt's.
			// If the previous stored values haven't expired and they don't match
			// the current pkt's, we will swap them to Table 2.
			if (table_1_6_entry != 0) {
				if (table_1_5_entry != 0) {
					// The entry hasn't yet expired, we need to compare the key.
					if (table_1_1_entry != 0 && table_1_1_entry != key_1) {
						to_swap_1 = true;
					} else if (table_1_2_entry != 0 && table_1_2_entry != key_2) {
						to_swap_1 = true;
					} else if (table_1_3_entry != 0 && table_1_3_entry != ports) {
						to_swap_1 = true;
					}
				}
			}

			// The previous Table 1 entry was occupied and not yet expired,
			// so we'll swap it to Table 2.
			if (to_swap_1) {
				bool to_swap_2 = false;

				bit<CUCKOO_HASH_BITS> idx_table_2_r = hash_table_2_recirc.get({table_1_1_entry,
																			   table_1_2_entry,
																			   table_1_3_entry});

				bit<32> table_2_1_entry = table_2_1_swap.execute(idx_table_2_r);
				bit<32> table_2_2_entry = table_2_2_swap.execute(idx_table_2_r);
				bit<32> table_2_3_entry = table_2_3_swap.execute(idx_table_2_r);

				ig_md.carry_swap_entry.entry_value	= table_2_4_swap.execute(idx_table_2_r);
				ig_md.carry_swap_entry.ts			= table_2_5_swap.execute(idx_table_2_r);
				ig_md.carry_swap_entry.ts_2			= table_2_6_swap.execute(idx_table_2_r);

				if (ig_md.carry_swap_entry.ts_2 != 0) {
					if (ig_md.carry_swap_entry.ts != 0) {
						// The entry hasn't yet expired, we need to compare the key.
						if (table_2_1_entry != 0) {
							to_swap_2 = true;
						} else if (table_2_2_entry != 0) {
							to_swap_2 = true;
						} else if (table_2_3_entry != 0) {
							to_swap_2 = true;
						}
					}
				}

				// The previous Table 2 entry was occupied and not yet expired,
				// so we'll recirculate and swap it to Table 1.
				if (to_swap_2) {
					// Cur pkt has been sucessfully inserted.
					// A swap entry is currently on ig_md.carry_swap_entry.*
					// During the bloom processing, the packet will be
					if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
						// First (re)circulation for the current pkt.
						// Store the swap entry values and change the op to SWAP.
						// The packet will be cloned later.

						hdr.ipv4.identification				= ip_flow_id;
						hdr.cuckoo_op.op					= cuckoo_ops_t.SWAP;

						ig_md.has_swap						= 1;
						ig_md.carry_swap_entry.ip_src_addr	= table_2_1_entry;
						ig_md.carry_swap_entry.ip_dst_addr	= table_2_2_entry;
						ig_md.carry_swap_entry.ports		= table_2_3_entry;
					} else if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAP) {
						// The current pkt is already a mirrored pkt.
						// Store the swap entry values and change the op to SWAPPED.
						// The packet will be cloned later, but won't be sent out.
						// (i.e., it already was sent out in a previous pipeline traversal).
						hdr.cuckoo_op.op = cuckoo_ops_t.SWAPPED;

						ig_md.has_swap = 1;
						ig_md.carry_swap_entry.ip_src_addr	= table_2_1_entry;
						ig_md.carry_swap_entry.ip_dst_addr	= table_2_2_entry;
						ig_md.carry_swap_entry.ports		= table_2_3_entry;
					}
				} else {
					// The previous Table 2 entry was expired/replaced.
					// In its place is now the cur pkt.
					if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
						// Send the current pkt to the bloom filter with op == NOP,
						// so it'll be sent out.
						hdr.ipv4.identification = ip_flow_id;
						hdr.cuckoo_op.op		= cuckoo_ops_t.NOP;
					} else if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAP) {
						// Send the current pkt to the bloom filter with op == SWAPPED,
						// to update the transient.
						hdr.cuckoo_op.op = cuckoo_ops_t.SWAPPED;
					}
				}
			} else {
				// The previous Table 1 entry was expired/replaced.
				// In its place is now the cur pkt.
				if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
					// Send the current pkt to the bloom filter with op == NOP,
					// so it'll be sent out.
					hdr.ipv4.identification = ip_flow_id;
					hdr.cuckoo_op.op		= cuckoo_ops_t.NOP;
				} else if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAP) {
					// Send the current pkt to the bloom filter with op == SWAPPED,
					// to update the transient.
					hdr.cuckoo_op.op = cuckoo_ops_t.SWAPPED;
				}
			}
		} else if ((hdr.cuckoo_op.isValid() && hdr.cuckoo_op.op == cuckoo_ops_t.LOOKUP) ||
				   (hdr.ipv4.isValid())) {
			if (hdr.ethernet.ether_type == ether_type_t.IPV4) {
				/* This is used by the parser to check if there are cuckoo headers */
				/* Also sets the recirculation cntr (last 16bits of dst mac address) */
				/* And the original ingress port (bits 24-16 of dst mac address) */
				hdr.ethernet.ether_type			= ether_type_t.CUCKOO;
				hdr.ethernet.dst_addr[15:0]		= 0x0;
				hdr.ethernet.dst_addr[24:16]	= ig_intr_md.ingress_port;
				original_ingress_port			= ig_intr_md.ingress_port;
			}

			bool table_1_match = false;
			bool table_2_match = false;

			bool table_1_1_lookup = table_1_1_lookup_action.execute(idx_table_1);
			bool table_1_2_lookup = table_1_2_lookup_action.execute(idx_table_1);

			if (table_1_1_lookup && table_1_2_lookup) {
				bit<32> table_1_3_lookup = table_1_3_read.execute(idx_table_1);
				if (table_1_3_lookup == ports) {
					table_1_match = true;
					// Update the stored ts values with the current ts.
					table_1_5_write.execute(idx_table_1);
					table_1_6_write.execute(idx_table_1);
				}
			}

			if (table_1_match) {
				// Do something with the value.
				// In this example we assign it to the IPv4 ID field.
				hdr.ipv4.identification = table_1_4_read.execute(idx_table_1);

				table_1_match_cntr_incr.execute(0);

				hdr.cuckoo_op.setValid();
				hdr.cuckoo_op.op = cuckoo_ops_t.NOP;
			} else {
				// Table 2 Lookup.
				bit<CUCKOO_HASH_BITS> idx_table_2 = hash_table_2.get({key_1, key_2, ports});

				bool table_2_1_lookup_bool = table_2_1_lookup.execute(idx_table_2);
				bool table_2_2_lookup_bool = table_2_2_lookup.execute(idx_table_2);

				if (table_2_1_lookup_bool && table_2_2_lookup_bool) {
					bit<32> table_2_3_lookup = table_2_3_read.execute(idx_table_2);

					if (table_2_3_lookup == ports) {
						table_2_match = true;
						table_2_5_write.execute(idx_table_2);
						table_2_6_write.execute(idx_table_2);
					}
				}

				if (table_2_match) {
					// Do something with the value.
					// In this example we assign it to the IPv4 ID field.
					hdr.ipv4.identification = table_2_4_read.execute(idx_table_2);

					table_2_match_cntr_incr.execute(0);

					hdr.cuckoo_op.setValid();
					hdr.cuckoo_op.op = cuckoo_ops_t.NOP;
				} else {
					// Lookup failed, recirculate the packet to insert it in the table.
					hdr.cuckoo_op.setValid();
					hdr.cuckoo_cntr.setValid();
					hdr.cuckoo_op.op		= cuckoo_ops_t.INSERT;
					ig_tm_md.packet_color	= 0;
				}
			}
		}

		// BLOOM

		bool transform_to_lookup	= false;

		if (hdr.cuckoo_op.isValid() && (ig_md.has_swap == 1 ||
										hdr.cuckoo_op.op == cuckoo_ops_t.INSERT ||
										hdr.cuckoo_op.op == cuckoo_ops_t.SWAPPED)) {
			// Handlers for transient states.
			bit<16> swap_transient_val		= 0;
			bit<16> swapped_transient_val	= 0;

			if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
				bit<SWAP_BLOOM_FILTER_HASH_BITS> swap_transient_idx =
						hash_swap.get({hdr.ipv4.src_addr,
									   hdr.ipv4.dst_addr,
									   ig_md.l4_lookup.src_port ++ ig_md.l4_lookup.dst_port});

				// If we've reached MAX_LOOPS_INSERT recirculations,
				// then assume that the previous swap pkt is lost and reset the transients.
				if (hdr.ethernet.dst_addr[15:0] == MAX_LOOPS_INSERT) {
					swap_transient_clear.execute(swap_transient_idx);
					swapped_transient_clear.execute(swap_transient_idx);
				} else {
					// Read the current transient values.
					swap_transient_val		= swap_transient_read.execute(swap_transient_idx);
					swapped_transient_val	= swapped_transient_read.execute(swap_transient_idx);
				}
			} else if (ig_md.has_swap == 0 && hdr.cuckoo_op.op == cuckoo_ops_t.SWAPPED) {
				// If op == SWAPPED and the pkt doesn't have a swap entry,
				// increment the swapped transient value.
				swapped_transient_incr.execute(
						hash_swapped.get({hdr.swap_entry.ip_src_addr,
										  hdr.swap_entry.ip_dst_addr,
										  hdr.swap_entry.ports}));
			} else if (ig_md.has_swap == 1) {
				// The current pkt has a swap entry.
				// Clone it and send it back to the Cuckoo pipe.
				ig_dprsr_md.mirror_type = SWAP_MIRROR;
				ig_md.mirror_session	= SWAP_MIRROR_SESSION;
				ig_md.swap_mirror.op	= cuckoo_ops_t.SWAP;

				hdr.cuckoo_cntr.has_swap	= 0;
				hdr.swap_entry.has_swap		= 0;

				// Increment the swap transient.
				swap_transient_incr.execute(
						hash_swap_2.get({ig_md.carry_swap_entry.ip_src_addr,
										 ig_md.carry_swap_entry.ip_dst_addr,
										 ig_md.carry_swap_entry.ports}));

				// Additionally, if the op == SWAPPED, increment the swapped transient.
				if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAPPED) {
					swapped_transient_incr.execute(
							hash_swapped_2.get({hdr.swap_entry.ip_src_addr,
												hdr.swap_entry.ip_dst_addr,
												hdr.swap_entry.ports}));
				}
			}

			if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAPPED) {
				// If op == SWAPPED, drop the current pkt.
				ig_dprsr_md.drop_ctl = 0x1;
			} else if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
				// If op == INSERT, the pkt will recirculate back to the cuckoo pipe.
				if (swap_transient_val != swapped_transient_val) {
					// However, if another pkt matching the same bloom idx is already
					// recirculating, the current pkt will be sent back as a LOOKUP.
					transform_to_lookup = true;
				}
			}
		}

		if (transform_to_lookup) {
			hdr.cuckoo_op.op = cuckoo_ops_t.LOOKUP;
		}

		if (hdr.cuckoo_op.op == cuckoo_ops_t.NOP || hdr.cuckoo_op.op == cuckoo_ops_t.SWAP) {
			hdr.ethernet.ether_type = ether_type_t.IPV4;
			fwd.apply();
		} else {
			hdr.ethernet.dst_addr[15:0] = hdr.ethernet.dst_addr[15:0] + 1;
			select_cuckoo_recirc_port.apply();
		}
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
	apply {
		/* Packets received here are copies of packets with this header sequence: */
		/* Swap Mirror | Ethernet | Cuckoo Op | (Swap Entry OR Cuckoo cntr) | Carry Swap Entry */
		/* We only need Swap Mirror info in the Cuckoo Pipe,
		 * so we copy data in the real Swap Entry and remove everything else */
		// hdr.cuckoo_op.setValid();
		// hdr.cuckoo_op.op = eg_md.swap_mirror.op;

		// hdr.swap_entry.setValid();
		// hdr.swap_entry.ip_src_addr		= ig_md.carry_swap_entry.ip_src_addr;
		// hdr.swap_entry.ip_dst_addr		= ig_md.carry_swap_entry.ip_dst_addr;
		// hdr.swap_entry.ports			= ig_md.carry_swap_entry.ports;
		// hdr.swap_entry.entry_value		= ig_md.carry_swap_entry.entry_value;
		// hdr.swap_entry.ts				= ig_md.carry_swap_entry.ts;
		// hdr.swap_entry.ts_2				= ig_md.carry_swap_entry.ts_2;
		// hdr.swap_entry.has_swap			= 0;

		// hdr.cuckoo_cntr.setInvalid();
	}
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
