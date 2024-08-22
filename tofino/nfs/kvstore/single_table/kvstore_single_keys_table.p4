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

#define HASH_SIZE_BITS 16
#define CACHE_CAPACITY (1 << HASH_SIZE_BITS)
#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds

#define KVSTORE_CAPACITY 10000
#define KEY_SIZE_BYTES 16
#define VALUE_SIZE_BYTES 128

typedef bit<8> op_t;
typedef bit<(KEY_SIZE_BYTES*8)> key_t;
typedef bit<(VALUE_SIZE_BYTES*8)> value_t;
typedef bit<8> status_t;
typedef bit<HASH_SIZE_BITS> hash_t;
typedef bit<32> time_t;

typedef bit<9> port_t;
typedef bit<7> port_pad_t;

const time_t EXPIRATION_TIME = 5 * ONE_SECOND;

const bit<16> TYPE_IPV4        = 0x800;
const bit<8>  IP_PROTOCOLS_TCP = 6;
const bit<8>  IP_PROTOCOLS_UDP = 17;

const port_t LAN = IN_PORT;
const port_t WAN = OUT_PORT;

const op_t OP_GET = 0;
const op_t OP_PUT = 1;
const op_t OP_DEL = 2;

const status_t STATUS_OK = 0;
const status_t STATUS_ERR = 1;

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
	bit<16> len;
	bit<16> checksum;
}

header kvstore_t {
	op_t op;
	key_t k;
	value_t v;
	status_t status;
}

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {}

struct my_ingress_headers_t {
	cpu_h cpu;
	ethernet_t ethernet;
	ipv4_t ipv4;
	udp_t udp;
	kvstore_t kvstore;
}

control Hashing(
	in  key_t  k,
	out hash_t hash
) {
	Hash<hash_t>(HashAlgorithm_t.CRC32) hash_calculator;

	action calc_hash() {
		hash = hash_calculator.get({k});
	}
	
	apply {
		calc_hash();
	}
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
	out ingress_intrinsic_metadata_t  ig_intr_md)
{
	TofinoIngressParser() tofino_parser;
	
	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, ig_intr_md);

		transition select(ig_intr_md.ingress_port) {
			CPU_PCIE_PORT: parse_cpu;
			default: parse_ethernet;
		}
	}

	state parse_cpu {
		pkt.extract(hdr.cpu);
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
			default: accept;
		}
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		transition parse_kvstore;
	}

	state parse_kvstore {
		pkt.extract(hdr.kvstore);
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
	inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{
	bit<16> port = 0;
	time_t now = 0;
	bit<32> key_index = 0;

	action forward(port_t p) {
		ig_tm_md.ucast_egress_port = p;
	}

	action send_to_controller() {
		hdr.cpu.setValid();
		hdr.cpu.code_path = 1234;
		hdr.cpu.in_port = ig_intr_md.ingress_port;
		forward(CPU_PCIE_PORT);
	}

	action populate(bit<16> out_port) {
		port = out_port;
	}

	table forwarder {
		key = {}

		actions = {
			populate;
		}

		size = 1;
	}

	action get_key_index(bit<32> _key_index) {
		key_index = _key_index;
	}

	table keys {
		key = {
			hdr.kvstore.k: exact;
		}

		actions = {
			get_key_index;
		}

		size = KVSTORE_CAPACITY;
	}

	// 128 bytes * 8 = 1024 bits
	// Each reg can store 32 bits
	// We need 32 registers to store 1024 bits

	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_0;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_1;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_2;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_3;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_4;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_5;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_6;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_7;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_8;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_9;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_10;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_11;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_12;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_13;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_14;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_15;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_16;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_17;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_18;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_19;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_20;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_21;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_22;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_23;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_24;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_25;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_26;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_27;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_28;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_29;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_30;
	Register<bit<32>, _>(KVSTORE_CAPACITY, 0) value_31;

	#define DEFINE_KVSTORE_VALUE_REG_READ(i) \
		RegisterAction<bit<32>, bit<32>, bit<32>>(value_##i) read_value_##i##_action = { \
			void apply(inout bit<32> curr_value, out bit<32> out_value) { \
				out_value = curr_value; \
			} \
		}; \
		action read_value_##i() { hdr.kvstore.v[(i+1)*32-1:i*32] = read_value_##i##_action.execute(key_index); }
	
	#define DEFINE_KVSTORE_VALUE_REG_WRITE(i) \
		RegisterAction<bit<32>, bit<32>, void>(value_##i) write_value_##i##_action = { \
			void apply(inout bit<32> curr_value) { \
				curr_value = hdr.kvstore.v[(i+1)*32-1:i*32]; \
			} \
		}; \
		action write_value_##i() { write_value_##i##_action.execute(key_index); }
	
	DEFINE_KVSTORE_VALUE_REG_READ(0)
	DEFINE_KVSTORE_VALUE_REG_READ(1)
	DEFINE_KVSTORE_VALUE_REG_READ(2)
	DEFINE_KVSTORE_VALUE_REG_READ(3)
	DEFINE_KVSTORE_VALUE_REG_READ(4)
	DEFINE_KVSTORE_VALUE_REG_READ(5)
	DEFINE_KVSTORE_VALUE_REG_READ(6)
	DEFINE_KVSTORE_VALUE_REG_READ(7)
	DEFINE_KVSTORE_VALUE_REG_READ(8)
	DEFINE_KVSTORE_VALUE_REG_READ(9)
	DEFINE_KVSTORE_VALUE_REG_READ(10)
	DEFINE_KVSTORE_VALUE_REG_READ(11)
	DEFINE_KVSTORE_VALUE_REG_READ(12)
	DEFINE_KVSTORE_VALUE_REG_READ(13)
	DEFINE_KVSTORE_VALUE_REG_READ(14)
	DEFINE_KVSTORE_VALUE_REG_READ(15)
	DEFINE_KVSTORE_VALUE_REG_READ(16)
	DEFINE_KVSTORE_VALUE_REG_READ(17)
	DEFINE_KVSTORE_VALUE_REG_READ(18)
	DEFINE_KVSTORE_VALUE_REG_READ(19)
	DEFINE_KVSTORE_VALUE_REG_READ(20)
	DEFINE_KVSTORE_VALUE_REG_READ(21)
	DEFINE_KVSTORE_VALUE_REG_READ(22)
	DEFINE_KVSTORE_VALUE_REG_READ(23)
	DEFINE_KVSTORE_VALUE_REG_READ(24)
	DEFINE_KVSTORE_VALUE_REG_READ(25)
	DEFINE_KVSTORE_VALUE_REG_READ(26)
	DEFINE_KVSTORE_VALUE_REG_READ(27)
	DEFINE_KVSTORE_VALUE_REG_READ(28)
	DEFINE_KVSTORE_VALUE_REG_READ(29)
	DEFINE_KVSTORE_VALUE_REG_READ(30)
	DEFINE_KVSTORE_VALUE_REG_READ(31)

	DEFINE_KVSTORE_VALUE_REG_WRITE(0)
	DEFINE_KVSTORE_VALUE_REG_WRITE(1)
	DEFINE_KVSTORE_VALUE_REG_WRITE(2)
	DEFINE_KVSTORE_VALUE_REG_WRITE(3)
	DEFINE_KVSTORE_VALUE_REG_WRITE(4)
	DEFINE_KVSTORE_VALUE_REG_WRITE(5)
	DEFINE_KVSTORE_VALUE_REG_WRITE(6)
	DEFINE_KVSTORE_VALUE_REG_WRITE(7)
	DEFINE_KVSTORE_VALUE_REG_WRITE(8)
	DEFINE_KVSTORE_VALUE_REG_WRITE(9)
	DEFINE_KVSTORE_VALUE_REG_WRITE(10)
	DEFINE_KVSTORE_VALUE_REG_WRITE(11)
	DEFINE_KVSTORE_VALUE_REG_WRITE(12)
	DEFINE_KVSTORE_VALUE_REG_WRITE(13)
	DEFINE_KVSTORE_VALUE_REG_WRITE(14)
	DEFINE_KVSTORE_VALUE_REG_WRITE(15)
	DEFINE_KVSTORE_VALUE_REG_WRITE(16)
	DEFINE_KVSTORE_VALUE_REG_WRITE(17)
	DEFINE_KVSTORE_VALUE_REG_WRITE(18)
	DEFINE_KVSTORE_VALUE_REG_WRITE(19)
	DEFINE_KVSTORE_VALUE_REG_WRITE(20)
	DEFINE_KVSTORE_VALUE_REG_WRITE(21)
	DEFINE_KVSTORE_VALUE_REG_WRITE(22)
	DEFINE_KVSTORE_VALUE_REG_WRITE(23)
	DEFINE_KVSTORE_VALUE_REG_WRITE(24)
	DEFINE_KVSTORE_VALUE_REG_WRITE(25)
	DEFINE_KVSTORE_VALUE_REG_WRITE(26)
	DEFINE_KVSTORE_VALUE_REG_WRITE(27)
	DEFINE_KVSTORE_VALUE_REG_WRITE(28)
	DEFINE_KVSTORE_VALUE_REG_WRITE(29)
	DEFINE_KVSTORE_VALUE_REG_WRITE(30)
	DEFINE_KVSTORE_VALUE_REG_WRITE(31)

	apply {
		if (hdr.cpu.isValid()) {
			forward(hdr.cpu.out_port);
			hdr.cpu.setInvalid();
		} else {
			now = ig_intr_md.ingress_mac_tstamp[47:16];
			forwarder.apply();
			forward(port[8:0]);

			bool found = keys.apply().hit;

			if (hdr.kvstore.op == OP_GET) {
				if (found) {
					read_value_0();
					read_value_1();
					read_value_2();
					read_value_3();
					read_value_4();
					read_value_5();
					read_value_6();
					read_value_7();
					read_value_8();
					read_value_9();
					read_value_10();
					read_value_11();
					read_value_12();
					read_value_13();
					read_value_14();
					read_value_15();
					read_value_16();
					read_value_17();
					read_value_18();
					read_value_19();
					read_value_20();
					read_value_21();
					read_value_22();
					read_value_23();
					read_value_24();
					read_value_25();
					read_value_26();
					read_value_27();
					read_value_28();
					read_value_29();
					read_value_30();
					read_value_31();
					hdr.kvstore.status = STATUS_OK;
				} else {
					hdr.kvstore.status = STATUS_ERR;
				}
			} else if (hdr.kvstore.op == OP_PUT) {
				if (!found) {
					send_to_controller();
				} else {
					write_value_0();
					write_value_1();
					write_value_2();
					write_value_3();
					write_value_4();
					write_value_5();
					write_value_6();
					write_value_7();
					write_value_8();
					write_value_9();
					write_value_10();
					write_value_11();
					write_value_12();
					write_value_13();
					write_value_14();
					write_value_15();
					write_value_16();
					write_value_17();
					write_value_18();
					write_value_19();
					write_value_20();
					write_value_21();
					write_value_22();
					write_value_23();
					write_value_24();
					write_value_25();
					write_value_26();
					write_value_27();
					write_value_28();
					write_value_29();
					write_value_30();
					write_value_31();
					hdr.kvstore.status = STATUS_OK;
				}
			} else if (hdr.kvstore.op == OP_DEL) {
				if (found) {
					send_to_controller();
				} else {
					hdr.kvstore.status = STATUS_ERR;
				}
			}

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
	in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
	apply {
		pkt.emit(hdr);
	}
}

parser TofinoEgressParser(
	packet_in pkt,
	out egress_intrinsic_metadata_t eg_intr_md)
{
  state start {
	pkt.extract(eg_intr_md);
	transition accept;
  }
}

parser EgressParser(
	packet_in pkt,
	out empty_header_t hdr,
	out empty_metadata_t eg_md,
	out egress_intrinsic_metadata_t eg_intr_md)
{
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
	inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md)
{
  apply {}
}

control EgressDeparser(
	packet_out pkt,
	inout empty_header_t hdr,
	in empty_metadata_t eg_md,
	in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md)
{
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
