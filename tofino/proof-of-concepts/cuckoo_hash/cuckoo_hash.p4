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
#endif

typedef bit<9> port_t;
typedef bit<7> port_pad_t;

const bit<16> TYPE_IPV4 = 0x800;

const bit<8> IP_PROTOCOLS_TCP = 6;
const bit<8> IP_PROTOCOLS_UDP = 17;
const bit<8> IP_PROTOCOLS_WARMUP = 146;

const bit<32> HASH_SALT_0 = 0xfbc31fc7;
const bit<32> HASH_SALT_1 = 0x2681580b;
const bit<32> HASH_SALT_2 = 0x486d7e2f;

#define CUCKOO_HASH_BITS 10
#define CUCKOO_CAPACITY (1 << CUCKOO_HASH_BITS)

#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds
#define EXPIRATION_TIME (5 * ONE_SECOND)

typedef bit<32> time_t;
typedef bit<CUCKOO_HASH_BITS> hash_t;

header cpu_h {
	bit<16> code_path;
	
	@padding port_pad_t pad0;
	port_t in_port;

	@padding port_pad_t pad1;
	port_t out_port;
}

header ethernet_t {
	bit<48> dstAddr;
	bit<48> srcAddr;
	bit<16> etherType;
}

header ipv4_t {
	bit<4>  version;
	bit<4>  ihl;
	bit<8>  diffserv;
	bit<16> total_len;
	bit<16> identification;
	bit<3>  flags;
	bit<13> frag_offset;
	bit<8>  ttl;
	bit<8>  protocol;
	bit<16> hdr_checksum;
	bit<32> src_addr;
	bit<32> dst_addr;
}

header udp_t {
	bit<16> src_port;
	bit<16> dst_port;
	bit<16> length;
	bit<16> checksum;
}

header kvs_t {
	bit<8> op;
	bit<128> key;
	bit<1024> value;
	bit<8> success;
	bit<8> tid;
	bit<8> already_inserted;
	bit<8> collision;
	bit<32> backtracks;
	bit<8> loop;
};

header recirc_h {
	bit<128> key;
	bit<1024> value;
};

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {
	bit<32> cuckoo_recirc_it;
}

struct my_ingress_headers_t {
	recirc_h recirc;
	cpu_h cpu;
	ethernet_t ethernet;
	ipv4_t ipv4;
	udp_t udp;
	kvs_t kvs;
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

		meta.cuckoo_recirc_it = 0;

		transition select(ig_intr_md.ingress_port) {
			CPU_PCIE_PORT: parse_cpu;
			RECIRCULATION_PORT: parse_recirculation;
			default: parse_ethernet;
		}
	}

	state parse_cpu {
		pkt.extract(hdr.cpu);
		transition parse_ethernet;
	}

	state parse_recirculation {
		meta.cuckoo_recirc_it = 1;
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.etherType) {
			TYPE_IPV4: parse_ipv4;
			default: accept;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select(hdr.ipv4.protocol) {
			IP_PROTOCOLS_UDP: parse_udp;
			IP_PROTOCOLS_WARMUP: parse_warmup;
			default: accept;
		}
	}

	state parse_warmup {
		meta.is_warmup_pkt = true;
		transition parse_udp;
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		transition parse_kvs;
	}

	state parse_kvs {
		kvs_t kvs;
		pkt.extract(kvs);
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

	hash_t h0;
	hash_t h1;

	action calc_hash0() {
		h0 = hash_calc0.get({
			hdr.kvs.key,
			HASH_SALT_0
		});
	}

	action calc_hash1() {
		h1 = hash_calc1.get({
			hdr.kvs.key,
			HASH_SALT_1
		});
	}

	// ============================== Expirator ==============================

	time_t now = 0;

	bool expirator_valid_t0 = false;
	bool expirator_valid_t1 = false;

	Register<time_t, _>(CUCKOO_CAPACITY, 0) expirator_t0;
	Register<time_t, _>(CUCKOO_CAPACITY, 0) expirator_t1;

	action set_now() {
		now = ig_intr_md.ingress_mac_tstamp[47:16];
	}

	RegisterAction<time_t, hash_t, bool>(expirator_t0) expirator_read_t0_action = {
		void apply(inout time_t alarm, out bool alive) {
			alive = false;
			if (now < alarm) {
				alive = true;
				alarm = now + EXPIRATION_TIME;
			}
		}
	};

	action expirator_read_t0() {
		expirator_valid_t0 = expirator_read_t0_action.execute(h0);
	}

	RegisterAction<time_t, hash_t, bool>(expirator_t1) expirator_read_t1_action = {
		void apply(inout time_t alarm, out bool alive) {
			alive = false;
			if (now < alarm) {
				alive = true;
				alarm = now + EXPIRATION_TIME;
			}
		}
	};

	action expirator_read_t1() {
		expirator_valid_t1 = expirator_read_t1_action.execute(h1);
	}

	RegisterAction<time_t, hash_t, bool>(expirator_t0) expirator_write_t0_action = {
		void apply(inout time_t alarm, out bool set) {
			set = false;
			if (now >= alarm) {
				set = true;
				alarm = now + EXPIRATION_TIME;
			}
		}
	};

	action expirator_write_t0() {
		expirator_valid_t0 = expirator_write_t0_action.execute(h0);
	}

	RegisterAction<time_t, hash_t, bool>(expirator_t1) expirator_write_t1_action = {
		void apply(inout time_t alarm, out bool set) {
			set = false;
			if (now >= alarm) {
				set = true;
				alarm = now + EXPIRATION_TIME;
			}
		}
	};

	action expirator_write_t1() {
		expirator_valid_t1 = expirator_write_t1_action.execute(h1);
	}

	RegisterAction<time_t, hash_t, bool>(expirator_t0) expirator_delete_t0_action = {
		void apply(inout time_t alarm, out bool alive) {
			alive = false;
			alarm = 0;
		}
	};

	action expirator_delete_t0() {
		expirator_valid_t0 = expirator_delete_t0_action.execute(h0);
	}

	RegisterAction<time_t, hash_t, bool>(expirator_t1) expirator_delete_t1_action = {
		void apply(inout time_t alarm, out bool alive) {
			alive = false;
			alarm = 0;
		}
	};

	action expirator_delete_t1() {
		expirator_valid_t1 = expirator_delete_t1_action.execute(h1);
	}

	// ============================== Cuckoo Hash ==============================

	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t0_k0_31;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t0_k32_63;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t0_k64_95;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t0_k96_127;

	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t1_k0_31;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t1_k32_63;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t1_k64_95;
	Register<bit<32>, _>(CUCKOO_CAPACITY, 0) t1_k96_127;

	bit<8> t0_eq_count = 0;
	bit<8> t1_eq_count = 0;

	RegisterAction<bit<32>, hash_t, bit<8>>(t0_k0_31) read_t0_k0_31_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[31:0]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t0_k0_31() {
		t0_eq_count = t0_eq_count + read_t0_k0_31_action.execute(h0);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t1_k0_31) read_t1_k0_31_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[31:0]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t1_k0_31() {
		t1_eq_count = t1_eq_count + read_t1_k0_31_action.execute(h1);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t0_k32_63) read_t0_k32_63_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[63:32]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t0_k32_63() {
		t0_eq_count = t0_eq_count + read_t0_k32_63_action.execute(h0);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t1_k32_63) read_t1_k32_63_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[63:32]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t1_k32_63() {
		t1_eq_count = t1_eq_count + read_t1_k32_63_action.execute(h1);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t0_k64_95) read_t0_k64_95_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[95:64]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t0_k64_95() {
		t0_eq_count = t0_eq_count + read_t0_k64_95_action.execute(h0);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t1_k64_95) read_t1_k64_95_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[95:64]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t1_k64_95() {
		t1_eq_count = t1_eq_count + read_t1_k64_95_action.execute(h1);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t0_k96_127) read_t0_k96_127_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[127:96]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t0_k96_127() {
		t0_eq_count = t0_eq_count + read_t0_k96_127_action.execute(h0);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t1_k96_127) read_t1_k96_127_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[127:96]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t1_k96_127() {
		t1_eq_count = t1_eq_count + read_t1_k96_127_action.execute(h1);
	}



















	RegisterAction<bit<32>, hash_t, void>(t0_k0_31) write_t0_k0_31_action = {
		void apply(inout bit<32> value) {
			value = hdr.kvs.key[31:0];
		}
	};

	action write_t0_k0_31() {
		write_t0_k0_31_action.execute(h0);
	}

	// TODO: finish the writes: 32-63, 64-95, 96-127

	RegisterAction<bit<32>, hash_t, bit<8>>(t1_k0_31) read_t1_k0_31_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[31:0]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t1_k0_31() {
		t1_eq_count = t1_eq_count + read_t1_k0_31_action.execute(h1);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t0_k32_63) read_t0_k32_63_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[63:32]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t0_k32_63() {
		t0_eq_count = t0_eq_count + read_t0_k32_63_action.execute(h0);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t1_k32_63) read_t1_k32_63_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[63:32]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t1_k32_63() {
		t1_eq_count = t1_eq_count + read_t1_k32_63_action.execute(h1);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t0_k64_95) read_t0_k64_95_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[95:64]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t0_k64_95() {
		t0_eq_count = t0_eq_count + read_t0_k64_95_action.execute(h0);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t1_k64_95) read_t1_k64_95_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[95:64]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t1_k64_95() {
		t1_eq_count = t1_eq_count + read_t1_k64_95_action.execute(h1);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t0_k96_127) read_t0_k96_127_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[127:96]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t0_k96_127() {
		t0_eq_count = t0_eq_count + read_t0_k96_127_action.execute(h0);
	}

	RegisterAction<bit<32>, hash_t, bit<8>>(t1_k96_127) read_t1_k96_127_action = {
		void apply(inout bit<32> value, out bit<8> result) {
			if (value == hdr.kvs.key[127:96]) {
				result = 1;
			} else {
				result = 0;
			}
		}
	};

	action read_t1_k96_127() {
		t1_eq_count = t1_eq_count + read_t1_k96_127_action.execute(h1);
	}

	action forward(port_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action send_to_controller() {
		hdr.cpu.setValid();
		hdr.cpu.code_path = 1234;
		hdr.cpu.in_port = ig_intr_md.ingress_port;
		forward(CPU_PCIE_PORT);
	}

	action recirculate() {
		forward(RECIRCULATION_PORT);
	}

	apply {
		calc_hash0();
		calc_hash1();

		set_now();

		if (hdr.recirc.isValid()) {
			expirator_write_t0();

			if (expirator_valid_t0) {
				write_t0_k0_31();
				write_t0_k32_63();
				write_t0_k64_95();
				write_t0_k96_127();

				hdr.kvs.tid = 0;
				hdr.kvs.success = 1;
			} else {
				expirator_write_t1();

				if (expirator_valid_t1) {
					write_t1_k0_31();
					write_t1_k32_63();
					write_t1_k64_95();
					write_t1_k96_127();

					hdr.kvs.tid = 1;
					hdr.kvs.success = 1;
				} else {
					recirculate();
					hdr.recirc.backtracks = hdr.recirc.backtracks + 1;
				}
			}
		} else {
			expirator_read_t0();
			expirator_read_t1();

			read_t0_k0_31();
			read_t0_k32_63();
			read_t0_k64_95();
			read_t0_k96_127();

			read_t1_k0_31();
			read_t1_k32_63();
			read_t1_k64_95();
			read_t1_k96_127();

			if (expirator_valid_t0 && t0_eq_count == 4) {
				hdr.kvs.tid = 0;
				if (hdr.kvs.op == 0) {
					hdr.kvs.success = 1;
				} else {
					hdr.kvs.success = 0;
					hdr.kvs.already_inserted = 1;
				}
			} else if (expirator_valid_t1 && t1_eq_count == 4) {
				hdr.kvs.tid = 1;
				if (hdr.kvs.op == 0) {
					hdr.kvs.success = 1;
				} else {
					hdr.kvs.success = 0;
					hdr.kvs.already_inserted = 1;
				}
			} else if (hdr.kvs.op == 1) {
				recirculate();
			}
		}

		forward(0);
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
