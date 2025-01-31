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
const bit<16> APP_KVS_PORT   = 670;

const bit<32> RANDOM0  = 0x219d424f;
const bit<32> RANDOM1  = 0x524040dd;
const bit<32> RANDOM2  = 0x6d69cc6e;
const bit<32> RANDOM3  = 0x33938a9d;
const bit<32> RANDOM4  = 0x7e2d7636;
const bit<32> RANDOM5  = 0xf316efeb;
const bit<32> RANDOM6  = 0x809b3db3;
const bit<32> RANDOM7  = 0xee27f7e5;
const bit<32> RANDOM8  = 0xa1d6aea8;
const bit<32> RANDOM9  = 0x318ac616;
const bit<32> RANDOM10 = 0x4d203493;
const bit<32> RANDOM11 = 0x56063af8;
const bit<32> RANDOM12 = 0xea1bea33;
const bit<32> RANDOM13 = 0x5ff33486;
const bit<32> RANDOM14 = 0xccc540f6;
const bit<32> RANDOM15 = 0x09063a97;
const bit<32> RANDOM16 = 0xa682c31b;
const bit<32> RANDOM17 = 0x7d100905;
const bit<32> RANDOM18 = 0xe6595272;
const bit<32> RANDOM19 = 0x00fe571a;
const bit<32> RANDOM20 = 0x120f62ab;
const bit<32> RANDOM21 = 0x988bc374;
const bit<32> RANDOM22 = 0xaca291b3;
const bit<32> RANDOM23 = 0xd0cb59fa;
const bit<32> RANDOM24 = 0xa1a642f2;
const bit<32> RANDOM25 = 0xa1c59ef8;
const bit<32> RANDOM26 = 0xd5afb9a4;
const bit<32> RANDOM27 = 0x8a071380;
const bit<32> RANDOM28 = 0x97f81272;
const bit<32> RANDOM29 = 0xe47cc718;
const bit<32> RANDOM30 = 0x101ad1b1;
const bit<32> RANDOM31 = 0x43123c96;

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
	bit<8> op;
	bit<96> key;
	bit<1024> value;
	bit<8> status;
}

struct headers_t {
	pktgen_timer_header_t timer;
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
		transition select (hdr.udp.dst_port) {
			APP_KVS_PORT: parse_kvs;
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
			drop;
		}

		default_action = drop;
		size = 32;
	}

	apply {
		router_tbl.apply();
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
		transition select (hdr.udp.dst_port) {
			APP_KVS_PORT: parse_kvs;
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
	Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_src_addr;
	Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_dst_addr;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash_src_port;
	Hash<bit<16>>(HashAlgorithm_t.CRC32) hash_dst_port;
	Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_kvs_key0;
	Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_kvs_key1;
	Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_kvs_key2;

	bit<32> salt = 0;

	action pick_salt0()  { salt = RANDOM0;  }
	action pick_salt1()  { salt = RANDOM1;  }
	action pick_salt2()  { salt = RANDOM2;  }
	action pick_salt3()  { salt = RANDOM3;  }
	action pick_salt4()  { salt = RANDOM4;  }
	action pick_salt5()  { salt = RANDOM5;  }
	action pick_salt6()  { salt = RANDOM6;  }
	action pick_salt7()  { salt = RANDOM7;  }
	action pick_salt8()  { salt = RANDOM8;  }
	action pick_salt9()  { salt = RANDOM9;  }
	action pick_salt10() { salt = RANDOM10; }
	action pick_salt11() { salt = RANDOM11; }
	action pick_salt12() { salt = RANDOM12; }
	action pick_salt13() { salt = RANDOM13; }
	action pick_salt14() { salt = RANDOM14; }
	action pick_salt15() { salt = RANDOM15; }
	action pick_salt16() { salt = RANDOM16; }
	action pick_salt17() { salt = RANDOM17; }
	action pick_salt18() { salt = RANDOM18; }
	action pick_salt19() { salt = RANDOM19; }
	action pick_salt20() { salt = RANDOM20; }
	action pick_salt21() { salt = RANDOM21; }
	action pick_salt22() { salt = RANDOM22; }
	action pick_salt23() { salt = RANDOM23; }
	action pick_salt24() { salt = RANDOM24; }
	action pick_salt25() { salt = RANDOM25; }
	action pick_salt26() { salt = RANDOM26; }
	action pick_salt27() { salt = RANDOM27; }
	action pick_salt28() { salt = RANDOM28; }
	action pick_salt29() { salt = RANDOM29; }
	action pick_salt30() { salt = RANDOM30; }
	action pick_salt31() { salt = RANDOM31; }

	table packet_modifier_tbl {
		key = {
			eg_intr_md.egress_port: exact;
		}

		actions = {
			pick_salt0;  pick_salt1;
			pick_salt2;  pick_salt3;
			pick_salt4;  pick_salt5;
			pick_salt6;  pick_salt7;
			pick_salt8;  pick_salt9;
			pick_salt10; pick_salt11;
			pick_salt12; pick_salt13;
			pick_salt14; pick_salt15;
			pick_salt16; pick_salt17;
			pick_salt18; pick_salt19;
			pick_salt20; pick_salt21;
			pick_salt22; pick_salt23;
			pick_salt24; pick_salt25;
			pick_salt26; pick_salt27;
			pick_salt28; pick_salt29;
			pick_salt30; pick_salt31;
		}

		size = 32;
	}

	action modify_src_addr() {
		hdr.ipv4.src_addr = hash_src_addr.get({ hdr.ipv4.src_addr, salt });
	}

	action modify_dst_addr() {
		hdr.ipv4.dst_addr = hash_dst_addr.get({ hdr.ipv4.dst_addr, salt });
	}

	action modify_src_port() {
		hdr.udp.src_port = hash_src_port.get({ hdr.udp.src_port, salt });
	}

	action modify_dst_port() {
		hdr.udp.dst_port = hash_dst_port.get({ hdr.udp.dst_port, salt });
	}

	action modify_kvs_key0() {
		hdr.kvs.key[31:0] = hash_kvs_key0.get({ hdr.kvs.key[31:0], salt });
	}

	action modify_kvs_key1() {
		hdr.kvs.key[63:32] = hash_kvs_key1.get({ hdr.kvs.key[63:32], salt });
	}

	action modify_kvs_key2() {
		hdr.kvs.key[95:64] = hash_kvs_key2.get({ hdr.kvs.key[95:64], salt });
	}

	apply {
		if (packet_modifier_tbl.apply().hit) {
			if (hdr.kvs.isValid()) {
				modify_src_addr();
				modify_src_port();
				
				modify_kvs_key0();
				modify_kvs_key1();
				modify_kvs_key2();
			} else {
				modify_src_addr();
				modify_dst_addr();
				modify_src_port();
				modify_dst_port();
			}
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
