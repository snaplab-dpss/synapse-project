#include <core.p4>

#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "includes/constants.p4"
#include "includes/parser.p4"
#include "includes/deparser.p4"
#include "includes/stats/cm.p4"
#include "includes/stats/bloom.p4"

// ---------------------------------------------------------------------------
// Pipeline - Ingress
// ---------------------------------------------------------------------------

control SwitchIngress(
		inout header_t hdr,
		inout ingress_metadata_t ig_md,
		in ingress_intrinsic_metadata_t ig_intr_md,
		in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
		inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
		inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

	Register<bit<8>, bit<16>>(NC_ENTRIES) reg_cache_lookup;

	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_k0_31;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_k32_63;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_k64_95;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_k96_127;

	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v0_31;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v32_63;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v64_95;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v96_127;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v128_159;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v160_191;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v192_223;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v224_255;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v256_287;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v288_319;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v320_351;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v352_383;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v384_415;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v416_447;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v448_479;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v480_511;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v512_543;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v544_575;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v576_607;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v608_639;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v640_671;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v672_703;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v704_735;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v736_767;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v768_799;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v800_831;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v832_863;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v864_895;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v896_927;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v928_959;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v960_991;
	Register<bit<32>, bit<16>>(NC_ENTRIES) reg_v992_1023;

	Hash<bit<16>>(HashAlgorithm_t.CRC16) hash_key;

	#define KEY_UPDATE(msb, lsb) \
		RegisterAction<bit<32>, bit<16>, bit<8>>(reg##_k##lsb##_##msb) update_k##lsb##_##msb## = { \
			void apply(inout bit<32> val) { \
				if (hdr.netcache.op == WRITE_QUERY) { \
					val = hdr.netcache.key[msb:lsb]; \
				} else { \
					val = 0; \
				} \
			} \
		};

	#define VAL_READ(msb, lsb) \
		RegisterAction<bit<32>, bit<16>, bit<32>>(reg##_v##lsb##_##msb) read_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = hdr.netcache.val[msb:lsb]; \
			} \
		};

	#define VAL_UPDATE(msb, lsb) \
		RegisterAction<bit<32>, bit<16>, bit<8>>(reg##_v##lsb##_##msb) update_v##lsb##_##msb## = { \
			void apply(inout bit<32> val) { \
				if (hdr.netcache.op == WRITE_QUERY) { \
					val = hdr.netcache.val[msb:lsb]; \
				} else { \
					val = 0; \
				} \
			} \
		};

	KEY_UPDATE(31, 0)
	KEY_UPDATE(63, 32)
	KEY_UPDATE(95, 64)
	KEY_UPDATE(127, 96)

	VAL_READ(31, 0)
	VAL_READ(63, 32)
	VAL_READ(95, 64)
	VAL_READ(127, 96)
	VAL_READ(159, 128)
	VAL_READ(191, 160)
	VAL_READ(223, 192)
	VAL_READ(255, 224)
	VAL_READ(287, 256)
	VAL_READ(319, 288)
	VAL_READ(351, 320)
	VAL_READ(383, 352)
	VAL_READ(415, 384)
	VAL_READ(447, 416)
	VAL_READ(479, 448)
	VAL_READ(511, 480)
	VAL_READ(543, 512)
	VAL_READ(575, 544)
	VAL_READ(607, 576)
	VAL_READ(639, 608)
	VAL_READ(671, 640)
	VAL_READ(703, 672)
	VAL_READ(735, 704)
	VAL_READ(767, 736)
	VAL_READ(799, 768)
	VAL_READ(831, 800)
	VAL_READ(863, 832)
	VAL_READ(895, 864)
	VAL_READ(927, 896)
	VAL_READ(959, 928)
	VAL_READ(991, 960)
	VAL_READ(1023, 992)

	VAL_UPDATE(31, 0)
	VAL_UPDATE(63, 32)
	VAL_UPDATE(95, 64)
	VAL_UPDATE(127, 96)
	VAL_UPDATE(159, 128)
	VAL_UPDATE(191, 160)
	VAL_UPDATE(223, 192)
	VAL_UPDATE(255, 224)
	VAL_UPDATE(287, 256)
	VAL_UPDATE(319, 288)
	VAL_UPDATE(351, 320)
	VAL_UPDATE(383, 352)
	VAL_UPDATE(415, 384)
	VAL_UPDATE(447, 416)
	VAL_UPDATE(479, 448)
	VAL_UPDATE(511, 480)
	VAL_UPDATE(543, 512)
	VAL_UPDATE(575, 544)
	VAL_UPDATE(607, 576)
	VAL_UPDATE(639, 608)
	VAL_UPDATE(671, 640)
	VAL_UPDATE(703, 672)
	VAL_UPDATE(735, 704)
	VAL_UPDATE(767, 736)
	VAL_UPDATE(799, 768)
	VAL_UPDATE(831, 800)
	VAL_UPDATE(863, 832)
	VAL_UPDATE(895, 864)
	VAL_UPDATE(927, 896)
	VAL_UPDATE(959, 928)
	VAL_UPDATE(991, 960)
	VAL_UPDATE(1023, 992)

	RegisterAction<_, bit<16>, bit<1>>(reg_cache_lookup) ract_cache_lookup = {
		void apply(inout bit<1> lookup, out bit<1> res) {
			res = lookup;
			if (hdr.netcache.op == WRITE_QUERY) {
				lookup = 1;
			} else if (hdr.netcache.op == DELETE_QUERY) {
				lookup = 0;
			}
		}
	};

	action hash_key_calc() {
		hdr.meta.hash_key = hash_key.get({hdr.netcache.key});
	}

	action cache_lookup() {
		hdr.meta.cache_hit = ract_cache_lookup.execute(hdr.meta.hash_key);
	}

	action update_pkt_udp() {
		bit<48> mac_src_tmp = hdr.ethernet.src_addr;
		hdr.ethernet.src_addr = hdr.ethernet.dst_addr;
		hdr.ethernet.dst_addr = mac_src_tmp;

		bit<32> ip_src_tmp = hdr.ipv4.src_addr;
		hdr.ipv4.src_addr = hdr.ipv4.dst_addr;
		hdr.ipv4.dst_addr = ip_src_tmp;

		bit<16> port_src_tmp = hdr.udp.src_port;
		hdr.udp.src_port = hdr.udp.dst_port;
		hdr.udp.dst_port = port_src_tmp;
	}

	action update_pkt_tcp() {
		bit<48> mac_src_tmp = hdr.ethernet.src_addr;
		hdr.ethernet.src_addr = hdr.ethernet.dst_addr;
		hdr.ethernet.dst_addr = mac_src_tmp;

		bit<32> ip_src_tmp = hdr.ipv4.src_addr;
		hdr.ipv4.src_addr = hdr.ipv4.dst_addr;
		hdr.ipv4.dst_addr = ip_src_tmp;

		bit<16> port_src_tmp = hdr.tcp.src_port;
		hdr.tcp.src_port = hdr.tcp.dst_port;
		hdr.tcp.dst_port = port_src_tmp;
	}

	action set_normal_pkt() {
		hdr.bridged_md.setValid();
		hdr.bridged_md.pkt_type = PKT_TYPE_NORMAL;
	}

	action set_out_port(PortId_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action miss() {}

	action set_client_packet() {
		hdr.meta.is_client_packet = 1;
	}

	action set_not_client_packet() {
		hdr.meta.is_client_packet = 0;
	}

	table is_client_packet {
		key = {
			ig_intr_md.ingress_port : exact;
		}
		actions = {
			set_client_packet;
			set_not_client_packet;
		}

		const default_action = set_client_packet;
		size = 2;
	}

	table fwd {
		key = {
			ig_intr_md.ingress_port : exact;
			hdr.meta.cache_hit      : exact;
			hdr.netcache.port		: ternary;
		}
		actions = {
			set_out_port;
			miss;
		}
		const default_action = miss;
		size = 1024;
	}

	apply {
		hdr.meta.setValid();
		hdr.meta.ingress_port = (bit<16>)ig_intr_md.ingress_port;
		is_client_packet.apply();

		// Calculate the hash based on the received pkt nc key.
		hash_key_calc();

		// Check if packet is not a HH report going from/to controller<->server.
		if (hdr.meta.is_client_packet == 1) {
			hdr.netcache.port = hdr.meta.ingress_port;
			cache_lookup();
			if (hdr.netcache.op == READ_QUERY) {
				if (hdr.meta.cache_hit == 1) {
					// Read the cached value and update the packet header.
					hdr.netcache.val[31:0] = read_v0_31.execute(hdr.meta.hash_key);
					hdr.netcache.val[63:32] = read_v32_63.execute(hdr.meta.hash_key);
					hdr.netcache.val[95:64] = read_v64_95.execute(hdr.meta.hash_key);
					hdr.netcache.val[127:96] = read_v96_127.execute(hdr.meta.hash_key);
					hdr.netcache.val[159:128] = read_v128_159.execute(hdr.meta.hash_key);
					hdr.netcache.val[191:160] = read_v160_191.execute(hdr.meta.hash_key);
					hdr.netcache.val[223:192] = read_v192_223.execute(hdr.meta.hash_key);
					hdr.netcache.val[255:224] = read_v224_255.execute(hdr.meta.hash_key);
					hdr.netcache.val[287:256] = read_v256_287.execute(hdr.meta.hash_key);
					hdr.netcache.val[319:288] = read_v288_319.execute(hdr.meta.hash_key);
					hdr.netcache.val[351:320] = read_v320_351.execute(hdr.meta.hash_key);
					hdr.netcache.val[383:352] = read_v352_383.execute(hdr.meta.hash_key);
					hdr.netcache.val[415:384] = read_v384_415.execute(hdr.meta.hash_key);
					hdr.netcache.val[447:416] = read_v416_447.execute(hdr.meta.hash_key);
					hdr.netcache.val[479:448] = read_v448_479.execute(hdr.meta.hash_key);
					hdr.netcache.val[511:480] = read_v480_511.execute(hdr.meta.hash_key);
					hdr.netcache.val[543:512] = read_v512_543.execute(hdr.meta.hash_key);
					hdr.netcache.val[575:544] = read_v544_575.execute(hdr.meta.hash_key);
					hdr.netcache.val[607:576] = read_v576_607.execute(hdr.meta.hash_key);
					hdr.netcache.val[639:608] = read_v608_639.execute(hdr.meta.hash_key);
					hdr.netcache.val[671:640] = read_v640_671.execute(hdr.meta.hash_key);
					hdr.netcache.val[703:672] = read_v672_703.execute(hdr.meta.hash_key);
					hdr.netcache.val[735:704] = read_v704_735.execute(hdr.meta.hash_key);
					hdr.netcache.val[767:736] = read_v736_767.execute(hdr.meta.hash_key);
					hdr.netcache.val[799:768] = read_v768_799.execute(hdr.meta.hash_key);
					hdr.netcache.val[831:800] = read_v800_831.execute(hdr.meta.hash_key);
					hdr.netcache.val[863:832] = read_v832_863.execute(hdr.meta.hash_key);
					hdr.netcache.val[895:864] = read_v864_895.execute(hdr.meta.hash_key);
					hdr.netcache.val[927:896] = read_v896_927.execute(hdr.meta.hash_key);
					hdr.netcache.val[959:928] = read_v928_959.execute(hdr.meta.hash_key);
					hdr.netcache.val[991:960] = read_v960_991.execute(hdr.meta.hash_key);
					hdr.netcache.val[1023:992] = read_v992_1023.execute(hdr.meta.hash_key);
					// Swap the IP src/dst and port src/dst.
					if (hdr.udp.isValid()) {
						update_pkt_udp();
					} else if (hdr.tcp.isValid()) {
						update_pkt_tcp();
					}
				}
			}

			else if (hdr.netcache.op == WRITE_QUERY || hdr.netcache.op == DELETE_QUERY) {
				// Update/delete the cached key/value with the received value.
				update_k0_31.execute(hdr.meta.hash_key);
				update_k32_63.execute(hdr.meta.hash_key);
				update_k64_95.execute(hdr.meta.hash_key);
				update_k96_127.execute(hdr.meta.hash_key);

				update_v0_31.execute(hdr.meta.hash_key);
				update_v32_63.execute(hdr.meta.hash_key);
				update_v64_95.execute(hdr.meta.hash_key);
				update_v96_127.execute(hdr.meta.hash_key);
				update_v128_159.execute(hdr.meta.hash_key);
				update_v160_191.execute(hdr.meta.hash_key);
				update_v192_223.execute(hdr.meta.hash_key);
				update_v224_255.execute(hdr.meta.hash_key);
				update_v256_287.execute(hdr.meta.hash_key);
				update_v288_319.execute(hdr.meta.hash_key);
				update_v320_351.execute(hdr.meta.hash_key);
				update_v352_383.execute(hdr.meta.hash_key);
				update_v384_415.execute(hdr.meta.hash_key);
				update_v416_447.execute(hdr.meta.hash_key);
				update_v448_479.execute(hdr.meta.hash_key);
				update_v480_511.execute(hdr.meta.hash_key);
				update_v512_543.execute(hdr.meta.hash_key);
				update_v544_575.execute(hdr.meta.hash_key);
				update_v576_607.execute(hdr.meta.hash_key);
				update_v608_639.execute(hdr.meta.hash_key);
				update_v640_671.execute(hdr.meta.hash_key);
				update_v672_703.execute(hdr.meta.hash_key);
				update_v704_735.execute(hdr.meta.hash_key);
				update_v736_767.execute(hdr.meta.hash_key);
				update_v768_799.execute(hdr.meta.hash_key);
				update_v800_831.execute(hdr.meta.hash_key);
				update_v832_863.execute(hdr.meta.hash_key);
				update_v864_895.execute(hdr.meta.hash_key);
				update_v896_927.execute(hdr.meta.hash_key);
				update_v928_959.execute(hdr.meta.hash_key);
				update_v960_991.execute(hdr.meta.hash_key);
				update_v992_1023.execute(hdr.meta.hash_key);
			}
		}

		set_normal_pkt();
		fwd.apply();
	}
}

// ---------------------------------------------------------------------------
// Pipeline - Egress
// ---------------------------------------------------------------------------

control SwitchEgress(
		inout header_t hdr,
		inout egress_metadata_t eg_md,
		in egress_intrinsic_metadata_t eg_intr_md,
		in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
		inout egress_intrinsic_metadata_for_deparser_t eg_dprsr_md,
		inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {

	c_cm()      cm;
	c_bloom()   bloom;

	Register<bit<8>, bit<1>>(1) reg_sampl;
	Register<bit<32>, bit<NC_KEY_WIDTH>>(NC_ENTRIES) reg_key_count;

	bit<8>  sampl_cur;
	bit<32> cm_result;
	bit<1>  bloom_result;

	RegisterAction<bit<8>, bit<8>, bit<8>>(reg_sampl) ract_sampl = {
		void apply(inout bit<8> val, out bit<8> res) {
			res = 0;
			if (val < 3) {
				val = val + 1;
			} else {
				val = 0;
				res = 1;
			}
		}
	};

	RegisterAction<_, bit<16>, bit<32>>(reg_key_count) ract_key_count_incr = {
		void apply(inout bit<32> val) {
			val = val + 1;
		}
	};

	action sampl_check() {
		sampl_cur = ract_sampl.execute(0);
	}

	action key_count_incr() {
		ract_key_count_incr.execute(hdr.meta.hash_key);
	}

	action set_mirror() {
		// eg_md.egr_mir_ses = hdr.bridged_md.egr_mir_ses;
		eg_md.egr_mir_ses = 128;
		eg_md.pkt_type = PKT_TYPE_MIRROR;
		eg_dprsr_md.mirror_type = MIRROR_TYPE_E2E;
		eg_dprsr_md.mirror_io_select = 1;
	}

	apply {
		// Packet is a HH report going from/to controller<->server or a server READ reply.
		if (hdr.meta.is_client_packet == 1 && hdr.netcache.op == READ_QUERY) {
			sampl_check();
			if (sampl_cur == 1) {
				if (hdr.meta.cache_hit == 1) {
					// Update cache counter.
					key_count_incr();
				} else {
					// Update cm sketch.
					cm.apply(hdr, cm_result);
					// Check cm result against threshold (HH_THRES).
					if (cm_result[31:7] != 0) {
						// Check against bloom filter.
						bloom.apply(hdr, bloom_result);
						// If confirmed HH, inform the controller through mirroring.
						if (bloom_result == 0) {
							hdr.netcache.key = (bit<NC_KEY_WIDTH>)hdr.meta.hash_key;
							hdr.netcache.val = (bit<NC_VAL_WIDTH>)cm_result;
							set_mirror();
						}
					}
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Instantiation
// ---------------------------------------------------------------------------

Pipeline(SwitchIngressParser(),
		SwitchIngress(),
		SwitchIngressDeparser(),
		SwitchEgressParser(),
		SwitchEgress(),
		SwitchEgressDeparser()) pipe;

Switch(pipe) main;
