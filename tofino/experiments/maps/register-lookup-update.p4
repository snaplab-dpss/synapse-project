/* -* P4_16 -*- */
#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif
typedef bit<9>  port_t;
typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> ether_type_t;
typedef bit<8>  ip_protocol_t;

const port_t RECIRCULATING_PORT = 68;

// Tofino model
const PortId_t CPU_PCIE_PORT = 64;

// BFN-T10-032D
// BFN-T10-032D-024
// BFN-T10-032D-020
// BFN-T10-032D-018
// const PortId_t CPU_PCIE_PORT = 192;

// BFN-T10-064Q
// BFN-T10-032Q
// const PortId_t CPU_PCIE_PORT = 320;

const port_t LAN = 0;
const port_t WAN = 1;

struct pair32 {
	bit<32> first;
	bit<32> second;
};

const ether_type_t  ETHERTYPE_IPV4     = 16w0x0800;
const ether_type_t  ETHERTYPE_ARP      = 16w0x0806;
const ether_type_t  ETHERTYPE_IPV6     = 16w0x86dd;
const ether_type_t  ETHERTYPE_VLAN     = 16w0x8100;

const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_TCP = 6;
const ip_protocol_t IP_PROTOCOLS_UDP = 17;

header cpu_h {
	bit<16> port;
	bit<16> code_path;
}

header ethernet_h {
	mac_addr_t   dst_addr;
	mac_addr_t   src_addr;
	ether_type_t ether_type;
}

header ipv4_h {
	bit<4>        version;
	bit<4>        ihl;
	bit<8>        dscp;
	bit<16>       total_len;
	bit<16>       identification;
	bit<3>        flags;
	bit<13>       frag_offset;
	bit<8>        ttl;
	ip_protocol_t protocol;
	bit<16>       hdr_checksum;
	ipv4_addr_t   src_addr;
	ipv4_addr_t   dst_addr;
}

header tcpudp_h {
	bit<16> src_port;
	bit<16> dst_port;
}

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {
	bit<32> map_value;
}

struct my_ingress_headers_t {
	cpu_h cpu;
	ethernet_h ethernet;
	ipv4_h     ipv4;
	tcpudp_h   tcpudp;
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
		meta.map_value = 0;

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

		transition select(hdr.ethernet.ether_type) {
			ETHERTYPE_IPV4: parse_ipv4;
			default: reject;
		}
	}

	state parse_ipv4 {
		pkt.extract(hdr.ipv4);

		// We only care about TCP packets
		transition select (hdr.ipv4.protocol) {
			IP_PROTOCOLS_TCP: parse_tcpudp;
			IP_PROTOCOLS_UDP: parse_tcpudp;
			default: reject;
		}
	}

	state parse_tcpudp {
		pkt.extract(hdr.tcpudp);
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
	bit<32> key_b0_b3;
	bit<32> key_b4_b7;
	bit<32> key_b8_b11;
	bit<16> key_hash;

	Hash<bit<16>>(HashAlgorithm_t.CRC16) hash;

	// Register entries in TNA may be bit<8>, int<8>, bit<16>,
	// int<16>, bit<32>, or int<32> values, or may be pairs of one of those types.

	// ============================ MAP CONTROL ================================

	Register<bit<8>, _>(65536) map_control;

	RegisterAction<bit<8>, _, bool>(map_control) map_control_check_and_reserve = {
		void apply(inout bit<8> is_reserved, out bool was_free) {
			if (is_reserved == 0) {
				is_reserved = 1;
				was_free = true;
			} else {
				was_free = false;
			}
		}
	};

	RegisterAction<bit<8>, _, bool>(map_control) map_control_check = {
		void apply(inout bit<8> is_reserved, out bool was_free) {
			if (is_reserved == 0) {
				was_free = true;
			} else {
				was_free = false;
			}
		}
	};

	// =================== MAP EXPIRATION TIME CONTROL =========================

	table map_exp_time {
		key = {
			key_hash: exact;
		}

		actions = {}

		size = 65536;
		idle_timeout = true;
	}

	// ============================ MAP KEYS ===================================

	Register<pair32, _>(65536) map_b0_b7_key;
	Register<bit<32>, _>(65536) map_b8_b11_key;

	RegisterAction<pair32, _, bool>(map_b0_b7_key) map_b0_b7_check = {
		void apply(inout pair32 value, out bool out_value) {
			if (value.first == key_b0_b3) {
				if (value.second == key_b4_b7) {
					out_value = true;
				} else {
					out_value = false;
				}
			} else {
				out_value = false;
			}
		}
	};

	RegisterAction<pair32, _, bool>(map_b0_b7_key) map_b0_b7_set = {
		void apply(inout pair32 value) {
			value.first = key_b0_b3;
			value.second = key_b4_b7;
		}
	};

	RegisterAction<bit<32>, _, bool>(map_b8_b11_key) map_b8_b11_check = {
		void apply(inout bit<32> value, out bool out_value) {
			if (value == key_b8_b11) {
				out_value = true;
			} else {
				out_value = false;
			}
		}
	};

	RegisterAction<bit<32>, _, bool>(map_b8_b11_key) map_b8_b11_set = {
		void apply(inout bit<32> value) {
			value = key_b8_b11;
		}
	};

	// ============================ MAP VALUE ==================================

	Register<bit<32>, _>(65536) map_b0_b7_value;

	RegisterAction<bit<32>, _, bit<32>>(map_b0_b7_value) map_value_get = {
		void apply(inout bit<32> value, out bit<32> out_value) {
			out_value = value;
		}
	};

	RegisterAction<bit<32>, _, bit<32>>(map_b0_b7_value) map_value_set = {
		void apply(inout bit<32> value){
			value = meta.map_value;
		}
	};

	// =========================================================================

	action fwd(port_t port){
		ig_tm_md.ucast_egress_port = port;
	}

	action drop() {
		ig_dprsr_md.drop_ctl = 1;
	}

	action send_to_cpu(){
		hdr.cpu.setValid();
		fwd(CPU_PCIE_PORT);
	}

	apply {
		if (hdr.cpu.isValid()) {
			// just forward packets coming from the CPU
			fwd((port_t)(hdr.cpu.port));
			return;
		}
		
		key_b0_b3 = hdr.ipv4.src_addr;
		key_b4_b7 = hdr.ipv4.dst_addr;

		key_b8_b11[15:0] = hdr.tcpudp.src_port;
		key_b8_b11[31:16] = hdr.tcpudp.dst_port;
		
		key_hash = hash.get({
			key_b0_b3,
			key_b4_b7,
			key_b8_b11
		});

		// map_get part 1

		// Be careful: this should only reserve in case we know we will
		// later write to this register.
		// This is very optimistic... What happens if later we can't 
		// write to the register due to a collision?
		bool was_free = map_control_check_and_reserve.execute(key_hash);
		
		if (was_free) {
			// map_get failure
			// map_put
			meta.map_value = 42;
			map_b0_b7_set.execute(key_hash);
			map_b8_b11_set.execute(key_hash);
			map_value_set.execute(key_hash);
		} else {
			// map_get part 2
			bool map_b0_b7_match = map_b0_b7_check.execute(key_hash);
			bool map_b8_b11_match = map_b8_b11_check.execute(key_hash);

			if (map_b0_b7_match && map_b8_b11_match) {
				// map_get success
				map_exp_time.apply();
				meta.map_value = map_value_get.execute(key_hash);
			} else {
				// NOT a map_get failure!
				// Register is filled, but with other data, i.e. collision
				send_to_cpu();
			}
		}
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
	apply {}
}

parser EgressParser(
		packet_in pkt,
		out empty_header_t hdr,
		out empty_metadata_t eg_md,
		out egress_intrinsic_metadata_t eg_intr_md) {
	state start {
		transition accept;
	}
}

control EgressDeparser(
		packet_out pkt,
		inout empty_header_t hdr,
		in empty_metadata_t eg_md,
		in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md) {
	apply {}
}

control Egress(
		inout empty_header_t hdr,
		inout empty_metadata_t eg_md,
		in egress_intrinsic_metadata_t eg_intr_md,
		in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
		inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
		inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
	apply {}
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
