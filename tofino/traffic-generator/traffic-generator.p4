#include <core.p4>

#if __TARGET_TOFINO__ == 2
	#include <t2na.p4>
#else
	#include <tna.p4>
#endif

typedef bit<9> port_t;
typedef bit<7> port_pad_t;
typedef bit<32> time_t;

const bit<16> ETHERTYPE_IPV4 = 0x800;
const bit<8>  IP_PROTO_TCP   = 6;
const bit<8>  IP_PROTO_UDP   = 17;
const bit<16> KVS_PORT        = 670;

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

header kvs_t {
	bit<8>  op;
	bit<32> key;
	bit<32> val;
	bit<8>  status;
	bit<16> port;
}


struct headers_t {
	ethernet_t ethernet;
	ipv4_t ipv4;
	udp_t udp;
	kvs_t kvs;
}

struct metadata_t {}

parser TofinoIngressParser(
	packet_in pkt,
	out headers_t hdr,
	out ingress_intrinsic_metadata_t ig_intr_md
) {
	state start {
		pkt.extract(ig_intr_md);
		transition select(ig_intr_md.resubmit_flag) {
			0: parse_port_metadata;
			1: parse_resubmit;
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
	out headers_t hdr,
	out metadata_t meta,
	out ingress_intrinsic_metadata_t ig_intr_md
) {
	TofinoIngressParser() tofino_parser;
	
	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, hdr, ig_intr_md);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.etherType) {
			ETHERTYPE_IPV4: parse_ipv4;
			default: reject;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select (hdr.ipv4.protocol) {
			IP_PROTO_UDP: parse_udp;
			default: reject;
		}
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		transition select(hdr.udp.src_port, hdr.udp.dst_port) {
			(KVS_PORT, _): parse_kvs;
			(_, KVS_PORT): parse_kvs;
			default: accept;
		}
	}

	state parse_kvs {
		pkt.extract(hdr.kvs);
		transition accept;
	}
}

control Ingress(
	inout headers_t hdr,
	inout metadata_t meta,
	in ingress_intrinsic_metadata_t ig_intr_md,
	in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
	inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
	inout ingress_intrinsic_metadata_for_tm_t ig_tm_md
) {
	action drop() {
		ig_dprsr_md.drop_ctl = 1;
	}

	action flood() {
		ig_tm_md.mcast_grp_a = 0x1;
	}
	
	action forward(port_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action bypass_egress() {
		ig_tm_md.bypass_egress = 1;
	}

	action swap32(inout bit<32> a, inout bit<32> b) {
		bit<32> tmp = a;
		a = b;
		b = tmp;
	}

	action swap16(inout bit<16> a, inout bit<16> b) {
		bit<16> tmp = a;
		a = b;
		b = tmp;
	}

	action route_tg() {
		flood();
	}

	action route_symmetric_response() {
		swap32(hdr.ipv4.src_addr, hdr.ipv4.dst_addr);
		swap16(hdr.udp.src_port, hdr.udp.dst_port);
		forward(ig_intr_md.ingress_port);
		bypass_egress();
	}

	table router_tbl {
		key = {
			ig_intr_md.ingress_port: exact;
		}

		actions = {
			route_tg;
			route_symmetric_response;
			forward;
			drop;
		}

		default_action = drop;
		size = 32;
	}

	Counter<bit<64>, bit<9>>(1024, CounterType_t.PACKETS_AND_BYTES) in_counter;

	apply {
		router_tbl.apply();
		in_counter.count(ig_intr_md.ingress_port);
	}
}

control IngressDeparser(
	packet_out pkt,
	inout headers_t hdr,
	in metadata_t meta,
	in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
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
	out headers_t hdr,
	out metadata_t meta,
	out egress_intrinsic_metadata_t eg_intr_md
) {
	TofinoEgressParser() tofino_parser;

	/* This is a mandatory state, required by Tofino Architecture */
	state start {
		tofino_parser.apply(pkt, eg_intr_md);
		transition parse_ethernet;
	}

	state parse_ethernet {
		pkt.extract(hdr.ethernet);
		transition select(hdr.ethernet.etherType) {
			ETHERTYPE_IPV4: parse_ipv4;
			default: reject;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);
		transition select (hdr.ipv4.protocol) {
			IP_PROTO_UDP: parse_udp;
			default: reject;
		}
	}

	state parse_udp {
		pkt.extract(hdr.udp);
		transition select(hdr.udp.src_port, hdr.udp.dst_port) {
			(KVS_PORT, _): parse_kvs;
			(_, KVS_PORT): parse_kvs;
			default: accept;
		}
	}

	state parse_kvs {
		pkt.extract(hdr.kvs);
		transition accept;
	}
}

control Egress(
	inout headers_t hdr,
	inout metadata_t meta,
	in egress_intrinsic_metadata_t eg_intr_md,
	in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
	inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md,
	inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
	Counter<bit<64>, bit<9>>(1024, CounterType_t.PACKETS_AND_BYTES) out_counter;

	bit<16> hash0_val = 0;
	bit<16> hash1_val = 0;
	bit<16> hash2_val = 0;
	bit<16> hash3_val = 0;
	bit<16> hash4_val = 0;

	const bit<16> HASH_SALT_0 = 0xfbc31fc7;
	const bit<16> HASH_SALT_1 = 0x2681580b;
	const bit<16> HASH_SALT_2 = 0x27a87100;
	const bit<16> HASH_SALT_3 = 0xbaf88a84;
	const bit<16> HASH_SALT_4 = 0xdf31859f;

	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash0_kvs;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash1_kvs;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash2_kvs;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash3_kvs;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash4_kvs;

	action hash0_calc_kvs() { hash0_val = hash0_kvs.get({ hdr.kvs.key, HASH_SALT_0 }); }
	action hash1_calc_kvs() { hash1_val = hash1_kvs.get({ hdr.kvs.key, HASH_SALT_1 }); }
	action hash2_calc_kvs() { hash2_val = hash2_kvs.get({ hdr.kvs.key, HASH_SALT_2 }); }
	action hash3_calc_kvs() { hash3_val = hash3_kvs.get({ hdr.kvs.key, HASH_SALT_3 }); }
	action hash4_calc_kvs() { hash4_val = hash4_kvs.get({ hdr.kvs.key, HASH_SALT_4 }); }

	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash0_non_kvs;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash1_non_kvs;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash2_non_kvs;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash3_non_kvs;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash4_non_kvs;

	action hash0_calc_non_kvs() { hash0_val = hash0_non_kvs.get({ hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.udp.src_port, hdr.udp.dst_port, HASH_SALT_0 }); }
	action hash1_calc_non_kvs() { hash1_val = hash1_non_kvs.get({ hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.udp.src_port, hdr.udp.dst_port, HASH_SALT_1 }); }
	action hash2_calc_non_kvs() { hash2_val = hash2_non_kvs.get({ hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.udp.src_port, hdr.udp.dst_port, HASH_SALT_2 }); }
	action hash3_calc_non_kvs() { hash3_val = hash3_non_kvs.get({ hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.udp.src_port, hdr.udp.dst_port, HASH_SALT_3 }); }
	action hash4_calc_non_kvs() { hash4_val = hash4_non_kvs.get({ hdr.ipv4.src_addr, hdr.ipv4.dst_addr, hdr.udp.src_port, hdr.udp.dst_port, HASH_SALT_4 }); }

	Register<bit<1>, _>(65536) flows_cms_row_0;
	Register<bit<1>, _>(65536) flows_cms_row_1;
	Register<bit<1>, _>(65536) flows_cms_row_2;
	Register<bit<1>, _>(65536) flows_cms_row_3;
	Register<bit<1>, _>(65536) flows_cms_row_4;

	RegisterAction<bit<1>, bit<16>, bit<1>>(flows_cms_row_0) flows_cms_row_0_update = {
		void apply(inout bit<1> val, out bit<1> res) {
			res = val;
			val = 1;
		}
	};

	RegisterAction<bit<1>, bit<16>, bit<1>>(flows_cms_row_1) flows_cms_row_1_update = {
		void apply(inout bit<1> val, out bit<1> res) {
			res = val;
			val = 1;
		}
	};

	RegisterAction<bit<1>, bit<16>, bit<1>>(flows_cms_row_2) flows_cms_row_2_update = {
		void apply(inout bit<1> val, out bit<1> res) {
			res = val;
			val = 1;
		}
	};

	RegisterAction<bit<1>, bit<16>, bit<1>>(flows_cms_row_3) flows_cms_row_3_update = {
		void apply(inout bit<1> val, out bit<1> res) {
			res = val;
			val = 1;
		}
	};

	RegisterAction<bit<1>, bit<16>, bit<1>>(flows_cms_row_4) flows_cms_row_4_update = {
		void apply(inout bit<1> val, out bit<1> res) {
			res = val;
			val = 1;
		}
	};

	bit<5> flows_cms_set = 0;
	action flows_cms_row_0_set() { flows_cms_set[0:0] = flows_cms_row_0_update.execute(hash0_val); }
	action flows_cms_row_1_set() { flows_cms_set[1:1] = flows_cms_row_1_update.execute(hash1_val); }
	action flows_cms_row_2_set() { flows_cms_set[2:2] = flows_cms_row_2_update.execute(hash2_val); }
	action flows_cms_row_3_set() { flows_cms_set[3:3] = flows_cms_row_3_update.execute(hash3_val); }
	action flows_cms_row_4_set() { flows_cms_set[4:4] = flows_cms_row_4_update.execute(hash4_val); }

	Register<bit<64>, _>(1) total_flows;

	RegisterAction<bit<32>, bit<1>, void>(total_flows) total_flows_increment = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	apply {
		out_counter.count(eg_intr_md.egress_port);

		if (hdr.kvs.isValid()) {
			hash0_calc_kvs();
			hash1_calc_kvs();
			hash2_calc_kvs();
			hash3_calc_kvs();
			hash4_calc_kvs();
		} else {
			hash0_calc_non_kvs();
			hash1_calc_non_kvs();
			hash2_calc_non_kvs();
			hash3_calc_non_kvs();
			hash4_calc_non_kvs();
		}

		flows_cms_row_0_set();
		flows_cms_row_1_set();
		flows_cms_row_2_set();
		flows_cms_row_3_set();
		flows_cms_row_4_set();

		if (flows_cms_set != 0b11111) {
			total_flows_increment.execute(0);
		}
	}
}

control EgressDeparser(
	packet_out pkt,
	inout headers_t hdr,
	in metadata_t meta,
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