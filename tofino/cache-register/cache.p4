#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

typedef bit<9>  port_t;

// hardware
// const port_t CPU_PCIE_PORT = 192;

// model
const port_t CPU_PCIE_PORT = 320;

const port_t IN_PORT = 0;
const port_t OUT_PORT = 1;

const port_t IN_DEV_PORT = 164;
const port_t OUT_DEV_PORT = 172;

header hdr0_h {
  bit<8> b0;
  bit<8> b1;
  bit<8> b2;
  bit<8> b3;
  bit<8> b4;
  bit<8> b5;
  bit<8> b6;
  bit<8> b7;
  bit<8> b8;
  bit<8> b9;
  bit<8> b10;
  bit<8> b11;
  bit<8> b12;
  bit<8> b13;
}

header hdr1_h {
  bit<8> b0;
  bit<8> b1;
  bit<8> b2;
  bit<8> b3;
  bit<8> b4;
  bit<8> b5;
  bit<8> b6;
  bit<8> b7;
  bit<8> b8;
  bit<8> b9;
  bit<8> b10;
  bit<8> b11;
  bit<8> b12;
  bit<8> b13;
  bit<8> b14;
  bit<8> b15;
  bit<8> b16;
  bit<8> b17;
  bit<8> b18;
  bit<8> b19;
}

header hdr2_h {
	bit<32> b0;
}

header cpu_h {
	bit<32> timestamp;
	bit<32> value;
	bit<32> result;
	bit<32> MAGIC_MARKER;
}

struct empty_header_t {}
struct empty_metadata_t {}

struct my_ingress_metadata_t {}

struct my_ingress_headers_t {
	hdr0_h hdr0;
	hdr1_h hdr1;
	hdr2_h hdr2;
	cpu_h cpu;
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
			default: parse_init;
		}
	}

	state parse_cpu {
		pkt.extract(hdr.cpu);
		transition parse_init;
	}

	state parse_init{
		transition parse_0;
	}

	state parse_0 {
		pkt.extract(hdr.hdr0);
		transition parse_1;
	}

	state parse_1{
		transition select(hdr.hdr0.b12++hdr.hdr0.b13) {
			16w2048: parse_2;
			default: reject;
		}
	}

	state parse_2 {
		pkt.extract(hdr.hdr1);
		transition parse_3;
	}

	state parse_3{
		transition select(hdr.hdr1.b9) {
			8w6: parse_4;
			8w17: parse_4;
			default: reject;
		}
	}

	state parse_4 {
		pkt.extract(hdr.hdr2);
		transition accept;
	}
}

#define ONE_SECOND 15258 // 1 second in units of 2**16 nanoseconds
#define EXPIRATION_TIME (5 * ONE_SECOND)
#define REGISTER_INDEX_WIDTH 10
#define REGISTER_CAPACITY 1024

struct cache_control_t {
	bit<32> last_time;
	bit<32> valid;
};

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
	bit<32> cache_key;
	bit<32> cache_value;
	bit<32> now;

	Hash<bit<REGISTER_INDEX_WIDTH>>(HashAlgorithm_t.CRC32) hash;

	Register<cache_control_t, _>(REGISTER_CAPACITY, {0, 0}) controls;
	Register<bit<32>, _>(REGISTER_CAPACITY) cache_keys;
	Register<bit<32>, _>(REGISTER_CAPACITY) cache_values;

	// ============================ CACHE CONTROL ================================

	RegisterAction<cache_control_t, _, bool>(controls) cache_control_check_update = {
		void apply(inout cache_control_t value, out bool reserved) {
			reserved = false;

			if (now < value.last_time || now > value.last_time + EXPIRATION_TIME) {
				value.last_time = now;
				value.valid = 1;
				reserved = true;
			}
		}
	};

	RegisterAction<cache_control_t, _, bool>(controls) cache_control_check = {
		void apply(inout cache_control_t value, out bool valid) {
			valid = false;

			if (value.valid == 1) {
				if (now > value.last_time + EXPIRATION_TIME) {
					value.valid = 0;
				} else {
					valid = true;
				}
			}
		}
	};

	// ============================ CACHE KEY ================================

	RegisterAction<bit<32>, _, bool>(cache_keys) cache_key_check = {
		void apply(inout bit<32> value, out bool match) {
			match = false;

			if (value == cache_key) {
				match = true;
			}
		}
	};

	RegisterAction<bit<32>, _, bool>(cache_keys) cache_key_update = {
		void apply(inout bit<32> value) {
			value = cache_key;
		}
	};

	// ============================ CACHE VALUE ================================

	RegisterAction<bit<32>, _, bit<32>>(cache_values) cache_value_get = {
		void apply(inout bit<32> value, out bit<32> out_value) {
			out_value = value;
		}
	};

	RegisterAction<bit<32>, _, bool>(cache_values) cache_value_update = {
		void apply(inout bit<32> value) {
			value = cache_value;
		}
	};

	// ============================ END CACHE ================================

	action fwd(port_t port, bit<32> timestamp, bit<32> value, bit<32> result) {
		ig_tm_md.ucast_egress_port = port;

		hdr.cpu.setValid();
		hdr.cpu.timestamp = timestamp;
		hdr.cpu.value = value;
		hdr.cpu.result = result;
		hdr.cpu.MAGIC_MARKER = 32w0xCAFEBABE;
	}

	action drop() {
		ig_dprsr_md.drop_ctl = 1;
	}

	apply {		
		bit<REGISTER_INDEX_WIDTH> key_hash = hash.get({
			hdr.hdr2.b0
		});

		cache_key = hdr.hdr2.b0;
		now = ig_intr_md.ingress_mac_tstamp[47:16];

		if (ig_intr_md.ingress_port == IN_DEV_PORT) {
			bool reserved = cache_control_check_update.execute(key_hash);

			if (reserved) {
				cache_key_update.execute(key_hash);

				cache_value = now;
				cache_value_update.execute(key_hash);

				fwd(OUT_DEV_PORT, now, cache_value, 2);
			} else {
				bit<32> value = cache_value_get.execute(key_hash);
				fwd(OUT_DEV_PORT, now, value, 4);
			}
		}
		
		else {
			bool valid = cache_control_check.execute(key_hash);
			bool match = false;

			if (valid) {
				match = cache_key_check.execute(key_hash);
			}

			if (match) {
				bit<32> value = cache_value_get.execute(key_hash);
				fwd(OUT_DEV_PORT, now, value, 3);
			} else {
				fwd(OUT_DEV_PORT, now, 0, 5);
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
