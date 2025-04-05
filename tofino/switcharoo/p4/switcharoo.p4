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

	Register<bit<32>, _>(1) cuckoo_recirculated_packets;
	Register<bit<32>, _>(1) insertions_counter;
	Register<bit<32>, _>(1) expired_entry_table_1;
	Register<bit<32>, _>(1) expired_entry_table_2;
	Register<bit<32>, _>(1) table_2_match_counter;
	Register<bit<32>, _>(1) table_1_match_counter;
	Register<bit<32>, _>(1) swap_creation;

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

	/* Bloom declarations */

	Register<bit<32>, _>(1) insert_counter_on_bloom;
	Register<bit<32>, _>(1) wait_counter_on_bloom;
	Register<bit<32>, _>(1) wait_max_loops_on_bloom;
	Register<bit<32>, _>(1) swap_counter_on_bloom;
	Register<bit<32>, _>(1) swapped_counter_on_bloom;
	Register<bit<32>, _>(1) lookup_counter_on_bloom;
	Register<bit<32>, _>(1) nop_counter_on_bloom;
	Register<bit<32>, _>(1) insert_max_loops_on_bloom;
	Register<bit<32>, _>(1) from_insert_to_lookup_swap;
	Register<bit<32>, _>(1) from_insert_to_lookup_bloom;
	Register<bit<32>, _>(1) from_nop_to_wait;

	Register<bit<32>, _>(1) bloom_packets_sent_out;
	Register<bit<32>, _>(1) swap_dropped;

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

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bool>(table_2_1) table_2_1_lookup_action = {
		void apply(inout bit<32> val, out bool res) {
			if (hdr.ipv4.src_addr == val) {
				res = true;
			} else {
				res = false;
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_HASH_BITS>, bool>(table_2_2) table_2_2_lookup_action = {
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

	RegisterAction<bit<32>, bit<1>, bit<32>>(cuckoo_recirculated_packets)
			cuckoo_recirculated_packets_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(insertions_counter) insertions_counter_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(expired_entry_table_1)
			expired_entry_table_1_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(expired_entry_table_2)
			expired_entry_table_2_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(table_2_match_counter)
			table_2_match_counter_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(table_1_match_counter)
			table_1_match_counter_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(swap_creation) swap_creation_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	/* BLOOM: STATS REGISTERS */

	RegisterAction<bit<32>, bit<1>, bit<32>>(bloom_packets_sent_out)
			bloom_packets_sent_out_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(wait_max_loops_on_bloom)
			wait_max_loops_on_bloom_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(insert_max_loops_on_bloom)
			insert_max_loops_on_bloom_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(from_insert_to_lookup_swap)
			from_insert_to_lookup_swap_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(from_insert_to_lookup_bloom)
			from_insert_to_lookup_bloom_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(wait_counter_on_bloom)
			wait_counter_on_bloom_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(insert_counter_on_bloom)
			insert_counter_on_bloom_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(swap_counter_on_bloom)
			swap_counter_on_bloom_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(swapped_counter_on_bloom)
			swapped_counter_on_bloom_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(nop_counter_on_bloom)
			nop_counter_on_bloom_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(lookup_counter_on_bloom)
			lookup_counter_on_bloom_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(swap_dropped) swap_dropped_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	RegisterAction<bit<32>, bit<1>, bit<32>>(from_nop_to_wait) from_nop_to_wait_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	/* BLOOM: Forward*/

	action send(PortId_t port_number) {
		ig_tm_md.ucast_egress_port	= port_number;
		ig_tm_md.bypass_egress		= 0x1;
		ig_dprsr_md.drop_ctl		= 0x0;
	}

	@ternary(1)
	table forward {
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
	Hash<bit<CUCKOO_HASH_BITS>>(HashAlgorithm_t.CUSTOM, poly_crc_16_dect2)	hash_table_2_recirculate;

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

	/* Port selections */
	/* Bloom Port Selection */

	// bit<9> original_ingress_port = hdr.ethernet.dst_addr[24:16];

	/*
	@ternary(1)
	table select_bloom_recirculation_port {
		key = {
			hdr.cuckoo_op.op		: exact;
			original_ingress_port	: exact;
		}
		actions = {
			recirculate_wait_ip1_to_bloom;
			recirculate_wait_ip2_to_bloom;
			recirculate_wait_ip3_to_bloom;
			recirculate_wait_ip4_to_bloom;
			recirculate_insert_ip1_to_bloom;
			recirculate_insert_ip2_to_bloom;
			recirculate_insert_ip3_to_bloom;
			recirculate_insert_ip4_to_bloom;
			recirculate_swapped_to_bloom;
			recirculate_nop_ip1_to_bloom;
			recirculate_nop_ip2_to_bloom;
			recirculate_nop_ip3_to_bloom;
			recirculate_nop_ip4_to_bloom;
		}
		size = 16;
		const entries = {
			(cuckoo_ops_t.WAIT, INGRESS_PORT_1)		: recirculate_wait_ip1_to_bloom();
			(cuckoo_ops_t.WAIT, INGRESS_PORT_2)		: recirculate_wait_ip2_to_bloom();
			(cuckoo_ops_t.WAIT, INGRESS_PORT_3)		: recirculate_wait_ip3_to_bloom();
			(cuckoo_ops_t.WAIT, INGRESS_PORT_4)		: recirculate_wait_ip4_to_bloom();
			(cuckoo_ops_t.INSERT, INGRESS_PORT_1)	: recirculate_insert_ip1_to_bloom();
			(cuckoo_ops_t.INSERT, INGRESS_PORT_2)	: recirculate_insert_ip2_to_bloom();
			(cuckoo_ops_t.INSERT, INGRESS_PORT_3)	: recirculate_insert_ip3_to_bloom();
			(cuckoo_ops_t.INSERT, INGRESS_PORT_4)	: recirculate_insert_ip4_to_bloom();
			(cuckoo_ops_t.SWAPPED, INGRESS_PORT_1)	: recirculate_swapped_to_bloom();
			(cuckoo_ops_t.SWAPPED, INGRESS_PORT_2)	: recirculate_swapped_to_bloom();
			(cuckoo_ops_t.SWAPPED, INGRESS_PORT_3)	: recirculate_swapped_to_bloom();
			(cuckoo_ops_t.SWAPPED, INGRESS_PORT_4)	: recirculate_swapped_to_bloom();
			(cuckoo_ops_t.NOP, INGRESS_PORT_1)		: recirculate_nop_ip1_to_bloom();
			(cuckoo_ops_t.NOP, INGRESS_PORT_2)		: recirculate_nop_ip2_to_bloom();
			(cuckoo_ops_t.NOP, INGRESS_PORT_3)		: recirculate_nop_ip3_to_bloom();
			(cuckoo_ops_t.NOP, INGRESS_PORT_4)		: recirculate_nop_ip4_to_bloom();
		}
	}
	*/

	/* Port selections */
	/* Cuckoo Port Selection */

	bit<9> original_ingress_port = hdr.ethernet.dst_addr[24:16];

	@ternary(1)
	table select_cuckoo_recirculation_port {
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
			/* Information is in the swap header */
			key_1		= hdr.swap_entry.ip_src_addr;
			key_2		= hdr.swap_entry.ip_dst_addr;
			ports		= hdr.swap_entry.ports;
			ip_flow_id	= hdr.swap_entry.entry_value;
			entry_ts	= hdr.swap_entry.ts;
			entry_ts_2	= hdr.swap_entry.ts_2;
		} else {
			/* Get everything from the original headers */
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
			/* Insert the value in Table 1.
			 * If the entry is not empty, swap the old value in Table 2 */
			cuckoo_recirculated_packets_increment.execute(0);

			if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
				/* First recirculation, compute a new flow ID */
				select_flow_id.apply();

				insertions_counter_increment.execute(0);
			}

			bool to_swap_1 = false;

			table_1_1_entry = table_1_1_swap.execute(idx_table_1);
			table_1_2_entry = table_1_2_swap.execute(idx_table_1);
			table_1_3_entry = table_1_3_swap.execute(idx_table_1);
			table_1_4_entry = table_1_4_swap.execute(idx_table_1);
			table_1_5_entry = table_1_5_swap.execute(idx_table_1);
			table_1_6_entry = table_1_6_swap.execute(idx_table_1);

			if (table_1_6_entry != 0) {
				/* First 16bits of timestamp are equal, check last 32bits */
				if (table_1_5_entry != 0) {
					/* Entry not expired, need to check if we must swap it to Table 2 */
					if (table_1_1_entry != 0 && table_1_1_entry != key_1) {
						to_swap_1 = true;
					} else if (table_1_2_entry != 0 && table_1_2_entry != key_2) {
						to_swap_1 = true;
					} else if (table_1_3_entry != 0 && table_1_3_entry != ports) {
						to_swap_1 = true;
					}
				} else {
					expired_entry_table_1_increment.execute(0);
				}
			} else {
				expired_entry_table_1_increment.execute(0);
			}

			if (to_swap_1) {
				/* Table 1 entry was not empty and not expired */
				bool to_swap_2 = false;

				bit<CUCKOO_HASH_BITS> idx_table_2_r = hash_table_2_recirculate.get({
						table_1_1_entry,
						table_1_2_entry,
						table_1_3_entry});

				bit<32> table_2_1_entry = table_2_1_swap.execute(idx_table_2_r);
				bit<32> table_2_2_entry = table_2_2_swap.execute(idx_table_2_r);
				bit<32> table_2_3_entry = table_2_3_swap.execute(idx_table_2_r);

				ig_md.carry_swap_entry.entry_value	= table_2_4_swap.execute(idx_table_2_r);
				ig_md.carry_swap_entry.ts			= table_2_5_swap.execute(idx_table_2_r);
				ig_md.carry_swap_entry.ts_2			= table_2_6_swap.execute(idx_table_2_r);

				if (ig_md.carry_swap_entry.ts_2 != 0) {
					/* First 16bits of timestamp are equal, check last 32bits */
					if (ig_md.carry_swap_entry.ts != 0) {
						 /* Entry not expired, need to check if we must swap it to Table 1 */
						if (table_2_1_entry != 0) {
							to_swap_2 = true;
						} else if (table_2_2_entry != 0) {
							to_swap_2 = true;
						} else if (table_2_3_entry != 0) {
							to_swap_2 = true;
						}
					} else {
						expired_entry_table_2_increment.execute(0);
					}
				} else {
					expired_entry_table_2_increment.execute(0);
				}

				if (to_swap_2) {
					swap_creation_increment.execute(0);

					/* Table 2 entry was not empty and not expired */
					if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
						/* If it is the first recirculation of this packet,
						 * we should send the original one to the bloom filter with
						 * swap.op = WAIT and append a swap_entry header which will
						 * create a copy on the Bloom to continue swapping */
						hdr.ipv4.identification = ip_flow_id;

						hdr.cuckoo_op.op = cuckoo_ops_t.WAIT;
						hdr.cuckoo_counter.recirc_counter = 0;

						hdr.cuckoo_counter.has_swap = 1;

						ig_md.has_swap = 1;
						ig_md.carry_swap_entry.ip_src_addr	= table_2_1_entry;
						ig_md.carry_swap_entry.ip_dst_addr	= table_2_2_entry;
						ig_md.carry_swap_entry.ports			= table_2_3_entry;
					} else if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAP) {
						/* If it's already a mirrored packet,
						 * send the original packet to the bloom with swap.op = SWAPPED,
						 * so it will decrease the corresponding bloom entry, and append a
						 * swap_entry header which will create a copy on the Bloom
						 * to continue swapping */
						hdr.cuckoo_op.op = cuckoo_ops_t.SWAPPED;

						hdr.swap_entry.has_swap = 1;

						ig_md.has_swap = 1;
						ig_md.carry_swap_entry.ip_src_addr	= table_2_1_entry;
						ig_md.carry_swap_entry.ip_dst_addr	= table_2_2_entry;
						ig_md.carry_swap_entry.ports			= table_2_3_entry;
					}
				} else {
					/* Table 2 entry was expired */
					if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
						/* Send the packet to the bloom filter with swap.op = WAIT.
						 * It will be recircultated on the Bloom Pipe until it is
						 * its turn to be sent out	*/
						hdr.ipv4.identification = ip_flow_id;

						hdr.cuckoo_op.op = cuckoo_ops_t.WAIT;
						hdr.cuckoo_counter.recirc_counter = 0;
					} else if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAP) {
						/* Send the packet to the Bloom Pipe with swap.op = SWAPPED, so it will decrease the bloom entry */
						hdr.cuckoo_op.op = cuckoo_ops_t.SWAPPED;
						ig_tm_md.packet_color = 0;
					}
				}
			} else {
				/* Table 1 was expired */
				if (hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
					/* Send the packet to the bloom filter with swap.op = WAIT. It will be recircultated on the Bloom Pipe
					until it is its turn to be sent out  */
					hdr.ipv4.identification = ip_flow_id;

					hdr.cuckoo_op.op = cuckoo_ops_t.WAIT;
					hdr.cuckoo_counter.recirc_counter = 0;
				} else if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAP) {
					/* Send the packet to the Bloom Pipe with swap.op = SWAPPED, so it will decrease the bloom entry */
					hdr.cuckoo_op.op = cuckoo_ops_t.SWAPPED;
					ig_tm_md.packet_color = 0;
				}
			}
		} else if ((hdr.cuckoo_op.isValid() && hdr.cuckoo_op.op == cuckoo_ops_t.LOOKUP) ||
				   (hdr.ipv4.isValid() && ig_md.first_frag == 1)) {
			if (hdr.ethernet.ether_type == ether_type_t.IPV4) {
				/* This is used by the parser to check if there are cuckoo headers */
				/* Also sets the recirculation counter (last 16bits of dst mac address) */
				/* And the original ingress port (bits 24-16 of dst mac address) */
				hdr.ethernet.ether_type			= ether_type_t.CUCKOO;
				hdr.ethernet.dst_addr[15:0]		= 0x0;
				hdr.ethernet.dst_addr[24:16]	= ig_intr_md.ingress_port;
				original_ingress_port			= ig_intr_md.ingress_port;
			}

			bool table_1_match = false;
			bool table_2_match = false;

			/* Lookup in Table 1 */
			bool table_1_1_lookup = table_1_1_lookup_action.execute(idx_table_1);
			bool table_1_2_lookup = table_1_2_lookup_action.execute(idx_table_1);
			if (table_1_1_lookup && table_1_2_lookup) {
				bit<32> table_1_3_lookup = table_1_3_read.execute(idx_table_1);
				if (table_1_3_lookup == ports) {
					table_1_match = true;
					// table_1_5.write(idx_table_1, ig_prsr_md.global_tstamp[31:0]);
					table_1_5_write.execute(idx_table_1);
					// table_1_6.write(idx_table_1, ig_prsr_md.global_tstamp[47:32]);
					table_1_6_write.execute(idx_table_1);
				}
			}

			if (table_1_match) {
				/* Do something with the value. In this example we assign it to the IPv4 ID field. */
				hdr.ipv4.identification = table_1_4_read.execute(idx_table_1);

				table_1_match_counter_increment.execute(0);

				if (hdr.cuckoo_op.isValid() && hdr.cuckoo_op.op == cuckoo_ops_t.LOOKUP) {
					/* Lookup was successful after a recirculation,
					 * send the packet to the Bloom Pipe with swap.op = WAIT */
					hdr.cuckoo_op.op = cuckoo_ops_t.WAIT;
					hdr.cuckoo_counter.recirc_counter = 0;
				} else {
					/* This packet never recirculated, send it to the Bloom Pipe with swap.op = NOP */
					hdr.cuckoo_op.setValid();
					hdr.cuckoo_op.op = cuckoo_ops_t.NOP;
				}
			} else {
				/* Lookup in Table 2 */
				bit<CUCKOO_HASH_BITS> idx_table_2 = hash_table_2.get({key_1, key_2, ports});

				bool table_2_1_lookup = table_2_1_lookup_action.execute(idx_table_2);
				bool table_2_2_lookup = table_2_2_lookup_action.execute(idx_table_2);
				if (table_2_1_lookup && table_2_2_lookup) {
					bit<32> table_2_3_lookup = table_2_3_read.execute(idx_table_2);
					if (table_2_3_lookup == ports) {
						table_2_match = true;
						table_2_5_write.execute(idx_table_2);
						table_2_6_write.execute(idx_table_2);
					}
				}

				if (table_2_match) {
					/* Do something with the value.
					 * In this example we assign it to the IPv4 ID field. */
					hdr.ipv4.identification = table_2_4_read.execute(idx_table_2);

					table_2_match_counter_increment.execute(0);

					if (hdr.cuckoo_op.isValid() && hdr.cuckoo_op.op == cuckoo_ops_t.LOOKUP) {
						/* Lookup was successful after a recirculation,
						 * send the packet to the Bloom Pipe with swap.op = WAIT */
						hdr.cuckoo_op.op = cuckoo_ops_t.WAIT;
						hdr.cuckoo_counter.recirc_counter = 0;
					} else {
						/* This packet never recirculated,
						 * send it to the Bloom Pipe with swap.op = NOP */
						hdr.cuckoo_op.setValid();
						hdr.cuckoo_op.op = cuckoo_ops_t.NOP;
					}
				} else {
					/* Lookup failed, recirculate the packet to insert it in the table */
					hdr.cuckoo_op.setValid();
					hdr.cuckoo_op.op = cuckoo_ops_t.INSERT;
					hdr.cuckoo_counter.setValid();
					hdr.cuckoo_counter.recirc_counter = 0;
					ig_tm_md.packet_color = 0;
				}
			}
		}

		// select_bloom_recirculation_port.apply();

		// BLOOM

		if (hdr.cuckoo_op.isValid() && hdr.cuckoo_op.op == cuckoo_ops_t.INSERT) {
			insert_counter_on_bloom_increment.execute(0);
		} else if (hdr.cuckoo_op.isValid() && hdr.cuckoo_op.op == cuckoo_ops_t.NOP) {
			nop_counter_on_bloom_increment.execute(0);
		} else if (hdr.cuckoo_op.isValid() && hdr.cuckoo_op.op == cuckoo_ops_t.SWAPPED) {
			swapped_counter_on_bloom_increment.execute(0);
		} else if (hdr.cuckoo_op.isValid() && hdr.cuckoo_op.op == cuckoo_ops_t.LOOKUP) {
			lookup_counter_on_bloom_increment.execute(0);
		}

		if (hdr.cuckoo_op.isValid() && (ig_md.has_swap == 1 ||
										hdr.cuckoo_op.op == cuckoo_ops_t.SWAPPED)) {
			if (ig_md.has_swap == 1) {
				/* This packet carries a swap entry */
				swap_counter_on_bloom_increment.execute(0);

				/* Clone and send to Cukoo pipe */
				ig_dprsr_md.mirror_type = SWAP_MIRROR;
				ig_md.mirror_session	= SWAP_MIRROR_SESSION;
				ig_md.swap_mirror.op	= cuckoo_ops_t.SWAP;

				hdr.cuckoo_counter.has_swap = 0;

				hdr.swap_entry.has_swap = 0;
			}

			if (hdr.cuckoo_op.op == cuckoo_ops_t.SWAPPED) {
				/* swap.op = SWAPPED must be dropped after
				 * updating the swapped_transient structure */
				ig_dprsr_md.drop_ctl = 0x1;
			}
		}

		if (hdr.cuckoo_op.op == cuckoo_ops_t.WAIT || hdr.cuckoo_op.op == cuckoo_ops_t.NOP) {
			bloom_packets_sent_out_increment.execute(0);


			hdr.cuckoo_op.setInvalid();
			hdr.cuckoo_counter.setInvalid();

			/* Restore original EtherType before sending out */
			hdr.ethernet.ether_type = ether_type_t.IPV4;

			forward.apply();
		} else {
			hdr.ethernet.dst_addr[15:0] = hdr.ethernet.dst_addr[15:0] + 1;

			select_cuckoo_recirculation_port.apply();
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
		/* Swap Mirror | Ethernet | Cuckoo Op | (Swap Entry OR Cuckoo Counter) | Carry Swap Entry */
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

		// hdr.cuckoo_counter.setInvalid();
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
