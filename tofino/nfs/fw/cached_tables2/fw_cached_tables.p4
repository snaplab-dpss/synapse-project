#include <core.p4>

#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "constants.p4"
#include "headers.p4"
#include "cached_table.p4"
#include "hashing.p4"

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
			IP_PROTOCOLS_TCP: parse_tcpudp;
			IP_PROTOCOLS_UDP: parse_tcpudp;
			default: accept;
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
	Hashing() hashing;

	bit<16> port;
	hash_t hash;
	bool hit;

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

	// ============================================================================

	match_counter_t key_match = 0;
    bool expirator_valid;
	time_t now;
	key_t k;
	value_t v;

    // ============================== Expirator ==============================

    Register<time_t, _>(CACHE_CAPACITY, 0) expirator;

    RegisterAction<time_t, hash_t, bool>(expirator) expirator_read_action = {
        void apply(inout time_t alarm, out bool alive) {
            alive = false;
            if (now < alarm) {
                alive = true;
                alarm = now + EXPIRATION_TIME;
            }
        }
    };

	RegisterAction<time_t, hash_t, bool>(expirator) expirator_read_action2 = {
        void apply(inout time_t alarm, out bool alive) {
            alive = false;
            if (now < alarm) {
                alive = true;
                alarm = now + EXPIRATION_TIME;
            }
        }
    };

    RegisterAction<time_t, hash_t, bool>(expirator) expirator_write_action = {
        void apply(inout time_t alarm, out bool alive) {
            alive = false;
            if (now < alarm) {
                alive = true;
            }
            alarm = now + EXPIRATION_TIME;
        }
    };

    RegisterAction<time_t, hash_t, bool>(expirator) expirator_delete_action = {
        void apply(inout time_t alarm, out bool alive) {
            alive = false;
            alarm = 0;
        }
    };

    action expirator_read() {
        expirator_valid = expirator_read_action.execute(hash);
    }

	action expirator_read2() {
        expirator_valid = expirator_read_action2.execute(hash);
    }

    action expirator_write() {
        expirator_valid = expirator_write_action.execute(hash);
    }

    action expirator_delete() {
        expirator_valid = expirator_delete_action.execute(hash);
    }

    // ============================== Cache ==============================

    Register<bit<32>, _>(CACHE_CAPACITY, 0) keys_src_addr;
	Register<bit<32>, _>(CACHE_CAPACITY, 0) keys_dst_addr;
	Register<bit<16>, _>(CACHE_CAPACITY, 0) keys_src_port;
	Register<bit<16>, _>(CACHE_CAPACITY, 0) keys_dst_port;

    Register<value_t, _>(CACHE_CAPACITY, 0) values;

    RegisterAction<bit<32>, hash_t, match_counter_t>(keys_src_addr) read_key_src_addr_action = {
		void apply(inout bit<32> curr_key, out match_counter_t match) {
			if (curr_key == k.src_addr) {
				match = 1;
			} else {
				match = 0;
			}
		}
	};

	RegisterAction<bit<32>, hash_t, match_counter_t>(keys_dst_addr) read_key_dst_addr_action = {
		void apply(inout bit<32> curr_key, out match_counter_t match) {
			if (curr_key == k.dst_addr) {
				match = 1;
			} else {
				match = 0;
			}
		}
	};

	RegisterAction<bit<16>, hash_t, match_counter_t>(keys_src_port) read_key_src_port_action = {
		void apply(inout bit<16> curr_key, out match_counter_t match) {
			if (curr_key == k.src_port) {
				match = 1;
			} else {
				match = 0;
			}
		}
	};

	RegisterAction<bit<16>, hash_t, match_counter_t>(keys_dst_port) read_key_dst_port_action = {
		void apply(inout bit<16> curr_key, out match_counter_t match) {
			if (curr_key == k.dst_port) {
				match = 1;
			} else {
				match = 0;
			}
		}
	};

    RegisterAction<value_t, hash_t, value_t>(values) read_value_action = {
        void apply(inout value_t curr_value, out value_t out_value) {
            out_value = curr_value;
        }
    };

    action read_key_src_addr() {
		key_match = key_match + read_key_src_addr_action.execute(hash);
	}

	action read_key_dst_addr() {
		key_match = key_match + read_key_dst_addr_action.execute(hash);
	}

	action read_key_src_port() {
		key_match = key_match + read_key_src_port_action.execute(hash);
	}

	action read_key_dst_port() {
		key_match = key_match + read_key_dst_port_action.execute(hash);
	}

    action read_value() {
        v = read_value_action.execute(hash);
    }

    RegisterAction<bit<32>, hash_t, void>(keys_src_addr) write_key_src_addr_action = {
		void apply(inout bit<32> curr_key) {
			curr_key = k.src_addr;
		}
	};

	RegisterAction<bit<32>, hash_t, void>(keys_dst_addr) write_key_dst_addr_action = {
		void apply(inout bit<32> curr_key) {
			curr_key = k.dst_addr;
		}
	};

	RegisterAction<bit<16>, hash_t, void>(keys_src_port) write_key_src_port_action = {
		void apply(inout bit<16> curr_key) {
			curr_key = k.src_port;
		}
	};

	RegisterAction<bit<16>, hash_t, void>(keys_dst_port) write_key_dst_port_action = {
		void apply(inout bit<16> curr_key) {
			curr_key = k.dst_port;
		}
	};

    RegisterAction<value_t, hash_t, void>(values) write_value_action = {
        void apply(inout value_t curr_value) {
            curr_value = v;
        }
    };

	action write_key_src_addr() {
		write_key_src_addr_action.execute(hash);
	}

	action write_key_dst_addr() {
		write_key_dst_addr_action.execute(hash);
	}

	action write_key_src_port() {
		write_key_src_port_action.execute(hash);
	}

	action write_key_dst_port() {
		write_key_dst_port_action.execute(hash);
	}

    action write_value() {
        write_value_action.execute(hash);
    }

    // ============================== Table ==============================

    action kv_map_get_value(value_t value) {
        v = value;
    }

    table kv_map {
		key = {
			k.src_addr: exact;
            k.dst_addr: exact;
            k.src_port: exact;
            k.dst_port: exact;
		}

		actions = {
			kv_map_get_value;
		}

		size = 65536;
	}

	bool kv_map_hit;

	table tmp {
		key = {
			kv_map_hit: exact;
		}

		actions = {
			expirator_read;
		}

		const entries = {
			(true): expirator_read();
		}

		size = 1;
	}

	apply {
		if (hdr.cpu.isValid()) {
			forward(hdr.cpu.out_port);
			hdr.cpu.setInvalid();
		} else {
			now = ig_intr_md.ingress_mac_tstamp[47:16];

			if (ig_intr_md.ingress_port == LAN) {
				forwarder.apply();

				k = {
					hdr.ipv4.src_addr,
					hdr.ipv4.dst_addr,
					hdr.tcpudp.src_port,
					hdr.tcpudp.dst_port
				};

				hashing.apply(k, hash);

				hit = false;
				kv_map_hit = kv_map.apply().hit;

				// if (kv_map_hit) {
				// 	hit = true;
				// } else {
				// 	expirator_read();
				// }

				tmp.apply();

				port = v;
				forward(port[8:0]);

				// if (!hit) {
				// 	send_to_controller();
				// } else {
				// 	forward(port[8:0]);
				// }
			} else {
				k = {
					hdr.ipv4.dst_addr,
					hdr.ipv4.src_addr,
					hdr.tcpudp.dst_port,
					hdr.tcpudp.src_port
				};

				hashing.apply(k, hash);

				hit = false;
				kv_map_hit = kv_map.apply().hit;
				
				tmp.apply();
				
				// if (kv_map_hit) {
				// 	hit = true;
				// } else {
				// 	expirator_read();
				// }

				port = v;
				forward(port[8:0]);

				// if (!hit) {
				// 	send_to_controller();
				// } else {
				// 	forward(port[8:0]);
				// }
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
