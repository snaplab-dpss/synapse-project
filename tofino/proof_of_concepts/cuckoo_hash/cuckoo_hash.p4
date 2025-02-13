#include <core.p4>

#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#if __TARGET_TOFINO__ == 2
#define CPU_PCIE_PORT 0

#define ETH_CPU_PORT_0 2
#define ETH_CPU_PORT_1 3
#define ETH_CPU_PORT_2 4
#define ETH_CPU_PORT_3 5

#define RECIRCULATION_PORT 6

// hardware
// #define IN_PORT 136
// #define OUT_PORT 144

// model
#define IN_PORT 8
#define OUT_PORT 9
#else
// hardware
// #define CPU_PCIE_PORT 192
// #define IN_PORT 164
// #define OUT_PORT 172

// model
#define CPU_PCIE_PORT 320
#define IN_PORT 0
#define OUT_PORT 1
#define RECIRCULATION_PORT 68
#endif

typedef bit<9> port_t;
typedef bit<7> port_pad_t;

const bit<16> TYPE_IPV4 = 0x800;

const bit<8> IP_PROTOCOLS_TCP = 6;
const bit<8> IP_PROTOCOLS_UDP = 17;

const bit<32> HASH_SALT_0 = 0x7f4a7c13;
const bit<32> HASH_SALT_1 = 0x97c29b3a;

#define CUCKOO_HASH_BITS 16
#define CUCKOO_CAPACITY (1 << CUCKOO_HASH_BITS)

#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds
#define EXPIRATION_TIME (1 * ONE_SECOND)

typedef bit<32> time_t;
typedef bit<CUCKOO_HASH_BITS> hash_t;

enum bit<8> cuckoo_op_t {
	LOOKUP = 0,
	INSERT = 1
}

header ethernet_t {
	bit<48> dstAddr;
	bit<48> srcAddr;
	bit<16> etherType;
}

header app_t {
	cuckoo_op_t op;
	bit<96> key;
};

header recirc_h {
	bit<32> times;
	bit<96> key;
	time_t swap_time;
};

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {
	bit<32> recirc_times;
	bit<96> key;
	time_t swap_time;
	time_t now;
	hash_t h0;
	hash_t h1;
	bool expirator_valid_t0;
	bool expirator_valid_t1;
	bit<8> t0_k0_31_entry;
	bit<8> t0_k32_63_entry;
	bit<8> t0_k64_95_entry;
	bit<8> t1_k0_31_entry;
	bit<8> t1_k32_63_entry;
	bit<8> t1_k64_95_entry;
	bit<8> tid;
	bit<8> success;
}

struct my_ingress_headers_t {
	recirc_h recirc;
	ethernet_t ethernet;
	app_t app;
}

parser TofinoIngressParser(
		packet_in pkt,
		out ingress_intrinsic_metadata_t ig_intr_md) {
	state start {
		pkt.extract(ig_intr_md);
		transition select(ig_intr_md.resubmit_flag) {
			1 : parse_resubmit;
			0 : parse_port_metadata;
		}
	}

	state parse_resubmit {
		// Parse resubmitted packet here.
		transition reject;
	}

	state parse_port_metadata {
		pkt.advance(PORT_METADATA_SIZE);
		transition accept;
	}
}

parser IngressParser(
	packet_in pkt,

	/* User */    
	out my_ingress_headers_t  hdr,
	out my_ingress_metadata_t meta,

	/* Intrinsic */
	out ingress_intrinsic_metadata_t  ig_intr_md
) {
	TofinoIngressParser() tofino_parser;
	
	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, ig_intr_md);

		meta.recirc_times = 0;
		meta.key = 0;
		meta.swap_time = 0;
		meta.now = ig_intr_md.ingress_mac_tstamp[47:16];
		meta.h0 = 0;
		meta.h1 = 0;
		meta.expirator_valid_t0 = false;
		meta.expirator_valid_t1 = false;
		meta.t0_k0_31_entry = 0;
		meta.t0_k32_63_entry = 0;
		meta.t0_k64_95_entry = 0;
		meta.t1_k0_31_entry = 0;
		meta.t1_k32_63_entry = 0;
		meta.t1_k64_95_entry = 0;
		meta.tid = 0;
		meta.success = 0;

		transition select(ig_intr_md.ingress_port) {
			RECIRCULATION_PORT: parse_recirculation;
			default: parse_ethernet;
		}
	}

	state parse_recirculation {
		pkt.extract(hdr.recirc);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition parse_app;
	}

	state parse_app {
		pkt.extract(hdr.app);
		transition accept;
	}
}

control Ingress(
	/* User */
	inout my_ingress_headers_t  hdr,
	inout my_ingress_metadata_t meta,

	/* Intrinsic */
	in    ingress_intrinsic_metadata_t               ig_intr_md,
	in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
	inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
	inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md
) {
	Hash<hash_t>(HashAlgorithm_t.CRC32) hash_calc0;
	Hash<hash_t>(HashAlgorithm_t.CRC32) hash_calc1;

	action calc_hash0() {
		meta.h0 = hash_calc0.get({
			HASH_SALT_0,
			meta.key
		});
	}

	action calc_hash1() {
		meta.h1 = hash_calc1.get({
			HASH_SALT_1,
			meta.key
		});
	}

	// ============================== Expirator ==============================

	Register<time_t, _>(CUCKOO_CAPACITY, 0) expirator_t0;
	Register<time_t, _>(CUCKOO_CAPACITY, 0) expirator_t1;

	RegisterAction<time_t, hash_t, bool>(expirator_t0) expirator_read_t0 = {
		void apply(inout time_t alarm, out bool alive) {
			if (meta.now < alarm) {
				alive = true;
			} else {
				alive = false;
			}
		}
	};

	RegisterAction<time_t, hash_t, bool>(expirator_t1) expirator_read_t1 = {
		void apply(inout time_t alarm, out bool alive) {
			if (meta.now < alarm) {
				alive = true;
			} else {
				alive = false;
			}
		}
	};

	RegisterAction<time_t, hash_t, time_t>(expirator_t0) expirator_swap_t0 = {
		void apply(inout time_t alarm, out time_t out_time) {
			if (meta.now >= alarm) {
				out_time = 0;
			} else {
				out_time = alarm;
			}
			alarm = meta.swap_time;
		}
	};

	RegisterAction<time_t, hash_t, time_t>(expirator_t1) expirator_swap_t1 = {
		void apply(inout time_t alarm, out time_t out_time) {
			if (meta.now >= alarm) {
				out_time = 0;
			} else {
				out_time = alarm;
			}
			alarm = meta.swap_time;
		}
	};

	// ============================== Cuckoo Hash ==============================

	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t0_k0_31;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t0_k32_63;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t0_k64_95;

	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t1_k0_31;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t1_k32_63;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t1_k64_95;

	#define CUCKOO_READ(num, msb, lsb) \
		RegisterAction<bit<32>, hash_t, bit<8>>(t##num##_k##lsb##_##msb) read_t##num##_k##lsb##_##msb## = { \
			void apply(inout bit<32> value, out bit<8> result) { \
				if (value == meta.key[msb:lsb]) { \
					result = 1; \
				} else { \
					result = 0; \
				} \
			} \
		};
	
	#define CUCKOO_SWAP(num, msb, lsb) \
		RegisterAction<bit<32>, hash_t, bit<32>>(t##num##_k##lsb##_##msb) swap_t##num##_k##lsb##_##msb## = { \
			void apply(inout bit<32> value, out bit<32> out_value) { \
				out_value = value; \
				value = meta.key[msb:lsb]; \
			} \
		};

	CUCKOO_READ(0, 31, 0)
	CUCKOO_READ(0, 63, 32)
	CUCKOO_READ(0, 95, 64)

	CUCKOO_SWAP(0, 31, 0)
	CUCKOO_SWAP(0, 63, 32)
	CUCKOO_SWAP(0, 95, 64)

	CUCKOO_READ(1, 31, 0)
	CUCKOO_READ(1, 63, 32)
	CUCKOO_READ(1, 95, 64)

	CUCKOO_SWAP(1, 31, 0)
	CUCKOO_SWAP(1, 63, 32)
	CUCKOO_SWAP(1, 95, 64)

	action forward(port_t port) {
		ig_tm_md.ucast_egress_port = port;
		hdr.recirc.setInvalid();
	}

	action recirculate() {
		forward(RECIRCULATION_PORT);
		hdr.recirc.setValid();
		hdr.recirc.key = meta.key;
		hdr.recirc.swap_time = meta.swap_time;
		hdr.recirc.times = meta.recirc_times + 1;
	}

	apply {
		bool trigger_recirculation = false;

		if (hdr.recirc.isValid()) {
			meta.key = hdr.recirc.key;
			meta.swap_time = hdr.recirc.swap_time;
			meta.recirc_times = hdr.recirc.times;

			calc_hash0();

			meta.swap_time = expirator_swap_t0.execute(meta.h0);

			meta.key[31:0] = swap_t0_k0_31.execute(meta.h0);
			meta.key[63:32] = swap_t0_k32_63.execute(meta.h0);
			meta.key[95:64] = swap_t0_k64_95.execute(meta.h0);

			if (meta.swap_time != 0) {
				calc_hash1();

				meta.swap_time = expirator_swap_t1.execute(meta.h1);
				
				meta.key[31:0] = swap_t1_k0_31.execute(meta.h1);
				meta.key[63:32] = swap_t1_k32_63.execute(meta.h1);
				meta.key[95:64] = swap_t1_k64_95.execute(meta.h1);

				bool is_original_key = true;
				if (meta.key[31:0] != hdr.app.key[31:0]) {
					is_original_key = false;
				} else if (meta.key[63:32] != hdr.app.key[63:32]) {
					is_original_key = false;
				} else if (meta.key[95:64] != hdr.app.key[95:64]) {
					is_original_key = false;
				}

				if (meta.swap_time != 0) {
					if (meta.recirc_times == 1 || !is_original_key) {
						trigger_recirculation = true;
					}
				} else {
					meta.success = 1;
				}
			}
		} else {
			meta.key = hdr.app.key;
			meta.swap_time = meta.now + EXPIRATION_TIME;

			calc_hash0();
			calc_hash1();

			meta.expirator_valid_t0 = expirator_read_t0.execute(meta.h0);

			meta.t0_k0_31_entry = read_t0_k0_31.execute(meta.h0);
			meta.t0_k32_63_entry = read_t0_k32_63.execute(meta.h0);
			meta.t0_k64_95_entry = read_t0_k64_95.execute(meta.h0);

			bool match0 = true;
			if (meta.t0_k0_31_entry == 0) {
				match0 = false;
			} else if (meta.t0_k32_63_entry == 0) {
				match0 = false;
			} else if (meta.t0_k64_95_entry == 0) {
				match0 = false;
			}

			meta.expirator_valid_t1 = expirator_read_t1.execute(meta.h1);

			meta.t1_k0_31_entry = read_t1_k0_31.execute(meta.h1);
			meta.t1_k32_63_entry = read_t1_k32_63.execute(meta.h1);
			meta.t1_k64_95_entry = read_t1_k64_95.execute(meta.h1);

			bool match1 = true;
			if (meta.t1_k0_31_entry == 0) {
				match1 = false;
			} else if (meta.t1_k32_63_entry == 0) {
				match1 = false;
			} else if (meta.t1_k64_95_entry == 0) {
				match1 = false;
			}

			if (meta.expirator_valid_t0 && match0) {
				if (hdr.app.op == cuckoo_op_t.LOOKUP) {
					meta.tid = 0;
					meta.success = 1;
				}
			} else if (meta.expirator_valid_t1 && match1) {
				meta.tid = 1;
				meta.success = 1;
			} else if (hdr.app.op == cuckoo_op_t.INSERT) {
				trigger_recirculation = true;
			}
		}

		if (trigger_recirculation) {
			recirculate();
		} else {
			forward(OUT_PORT);
		}

		ig_tm_md.bypass_egress = 1;
	}
}

control IngressDeparser(
	packet_out pkt,

	/* User */
	inout my_ingress_headers_t  hdr,
	in    my_ingress_metadata_t meta,

	/* Intrinsic */
	in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md
) {
	apply {
		pkt.emit(hdr);
	}
}

parser TofinoEgressParser(
	packet_in pkt,
	out egress_intrinsic_metadata_t eg_intr_md
) {
	state start {
		pkt.extract(eg_intr_md);
		transition accept;
	}
}

parser EgressParser(
	packet_in pkt,
	out empty_header_t hdr,
	out empty_metadata_t eg_md,
	out egress_intrinsic_metadata_t eg_intr_md
) {
	TofinoEgressParser() tofino_parser;

	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, eg_intr_md);
		transition accept;
	}
}

control Egress(
	inout empty_header_t hdr,
	inout empty_metadata_t eg_md,
	in egress_intrinsic_metadata_t eg_intr_md,
	in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
	inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
	inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
	apply {}
}

control EgressDeparser(
	packet_out pkt,
	inout empty_header_t hdr,
	in empty_metadata_t eg_md,
	in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md
) {
	apply {
		pkt.emit(hdr);
	}
}

Pipeline(
	IngressParser(),
	Ingress(),
	IngressDeparser(),
	EgressParser(),
	Egress(),
	EgressDeparser()
) pipe;

Switch(pipe) main;
