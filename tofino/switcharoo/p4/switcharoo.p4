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

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_k0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_k32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_k64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_k96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_k0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_k32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_k64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_k96_127;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v96_127;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v128_159;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v160_191;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v192_223;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v224_255;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v256_287;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v288_319;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v320_351;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v352_383;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v384_415;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v416_447;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v448_479;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v480_511;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v512_543;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v544_575;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v576_607;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v608_639;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v640_671;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v672_703;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v704_735;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v736_767;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v768_799;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v800_831;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v832_863;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v864_895;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v896_927;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v928_959;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v960_991;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_1_v992_1023;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v0_31;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v32_63;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v64_95;
	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v96_127;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v128_159;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v160_191;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v192_223;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v224_255;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v256_287;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v288_319;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v320_351;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v352_383;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v384_415;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v416_447;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v448_479;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v480_511;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v512_543;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v544_575;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v576_607;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v608_639;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v640_671;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v672_703;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v704_735;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v736_767;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v768_799;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v800_831;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v832_863;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v864_895;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v896_927;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v928_959;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v960_991;
	// Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_2_v992_1023;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_table_1_ts_1;
	Register<bit<16>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_table_1_ts_2;

	Register<bit<32>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_table_2_ts_1;
	Register<bit<16>, bit<CUCKOO_IDX_WIDTH>>(CUCKOO_ENTRIES) reg_table_2_ts_2;

	bit<32> table_1_ts_1_entry	= 0;
	bit<16> table_1_ts_2_entry	= 0;
	bit<32> table_2_ts_1_entry	= 0;
	bit<16> table_2_ts_2_entry	= 0;

	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swap_transient;
	Register<bit<16>, bit<BLOOM_IDX_WIDTH>>(BLOOM_ENTRIES) swapped_transient;

	#define KEY_READ(table, msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_##table##_k##lsb##_##msb) read_##table##_k##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
			} \
		};

	#define KEY_SWAP_1(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1##_k##lsb##_##msb) swap_1##_k##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val = ig_md.cur_key[msb:lsb]; \
			} \
		};

	#define KEY_SWAP_2(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2##_k##lsb##_##msb) swap_2##_k##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val = ig_md.table_1_key[msb:lsb]; \
			} \
		};

	#define VAL_READ(table, msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_##table##_v##lsb##_##msb) read_##table##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
			} \
		};

	#define VAL_WRITE(table, msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_##table##_v##lsb##_##msb) write_##table##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val) { \
				val = ig_md.cur_val[msb:lsb]; \
			} \
		};

	#define VAL_SWAP_1(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_1##_v##lsb##_##msb) swap_1##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val = ig_md.cur_val[msb:lsb]; \
			} \
		};

	#define VAL_SWAP_2(msb, lsb) \
		RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_2##_v##lsb##_##msb) swap_2##_v##lsb##_##msb## = { \
			void apply(inout bit<32> val, out bit<32> res) { \
				res = val; \
				val= ig_md.table_1_val[msb:lsb]; \
			} \
		};

	KEY_READ(1, 31, 0)
	KEY_READ(1, 63, 32)
	KEY_READ(1, 95, 64)
	KEY_READ(1, 127, 96)

	KEY_READ(2, 31, 0)
	KEY_READ(2, 63, 32)
	KEY_READ(2, 95, 64)
	KEY_READ(2, 127, 96)

	KEY_SWAP_1(31, 0)
	KEY_SWAP_1(63, 32)
	KEY_SWAP_1(95, 64)
	KEY_SWAP_1(127, 96)

	KEY_SWAP_2(31, 0)
	KEY_SWAP_2(63, 32)
	KEY_SWAP_2(95, 64)
	KEY_SWAP_2(127, 96)

	VAL_READ(1, 31, 0)
	VAL_READ(1, 63, 32)
	VAL_READ(1, 95, 64)
	VAL_READ(1, 127, 96)
	// VAL_READ(1, 159, 128)
	// VAL_READ(1, 191, 160)
	// VAL_READ(1, 223, 192)
	// VAL_READ(1, 255, 224)
	// VAL_READ(1, 287, 256)
	// VAL_READ(1, 319, 288)
	// VAL_READ(1, 351, 320)
	// VAL_READ(1, 383, 352)
	// VAL_READ(1, 415, 384)
	// VAL_READ(1, 447, 416)
	// VAL_READ(1, 479, 448)
	// VAL_READ(1, 511, 480)
	// VAL_READ(1, 543, 512)
	// VAL_READ(1, 575, 544)
	// VAL_READ(1, 607, 576)
	// VAL_READ(1, 639, 608)
	// VAL_READ(1, 671, 640)
	// VAL_READ(1, 703, 672)
	// VAL_READ(1, 735, 704)
	// VAL_READ(1, 767, 736)
	// VAL_READ(1, 799, 768)
	// VAL_READ(1, 831, 800)
	// VAL_READ(1, 863, 832)
	// VAL_READ(1, 895, 864)
	// VAL_READ(1, 927, 896)
	// VAL_READ(1, 959, 928)
	// VAL_READ(1, 991, 960)
	// VAL_READ(1, 1023, 992)

	VAL_READ(2, 31, 0)
	VAL_READ(2, 63, 32)
	VAL_READ(2, 95, 64)
	VAL_READ(2, 127, 96)
	// VAL_READ(2, 159, 128)
	// VAL_READ(2, 191, 160)
	// VAL_READ(2, 223, 192)
	// VAL_READ(2, 255, 224)
	// VAL_READ(2, 287, 256)
	// VAL_READ(2, 319, 288)
	// VAL_READ(2, 351, 320)
	// VAL_READ(2, 383, 352)
	// VAL_READ(2, 415, 384)
	// VAL_READ(2, 447, 416)
	// VAL_READ(2, 479, 448)
	// VAL_READ(2, 511, 480)
	// VAL_READ(2, 543, 512)
	// VAL_READ(2, 575, 544)
	// VAL_READ(2, 607, 576)
	// VAL_READ(2, 639, 608)
	// VAL_READ(2, 671, 640)
	// VAL_READ(2, 703, 672)
	// VAL_READ(2, 735, 704)
	// VAL_READ(2, 767, 736)
	// VAL_READ(2, 799, 768)
	// VAL_READ(2, 831, 800)
	// VAL_READ(2, 863, 832)
	// VAL_READ(2, 895, 864)
	// VAL_READ(2, 927, 896)
	// VAL_READ(2, 959, 928)
	// VAL_READ(2, 991, 960)
	// VAL_READ(2, 1023, 992)

	VAL_WRITE(1, 31, 0)
	VAL_WRITE(1, 63, 32)
	VAL_WRITE(1, 95, 64)
	VAL_WRITE(1, 127, 96)
	// VAL_WRITE(1, 159, 128)
	// VAL_WRITE(1, 191, 160)
	// VAL_WRITE(1, 223, 192)
	// VAL_WRITE(1, 255, 224)
	// VAL_WRITE(1, 287, 256)
	// VAL_WRITE(1, 319, 288)
	// VAL_WRITE(1, 351, 320)
	// VAL_WRITE(1, 383, 352)
	// VAL_WRITE(1, 415, 384)
	// VAL_WRITE(1, 447, 416)
	// VAL_WRITE(1, 479, 448)
	// VAL_WRITE(1, 511, 480)
	// VAL_WRITE(1, 543, 512)
	// VAL_WRITE(1, 575, 544)
	// VAL_WRITE(1, 607, 576)
	// VAL_WRITE(1, 639, 608)
	// VAL_WRITE(1, 671, 640)
	// VAL_WRITE(1, 703, 672)
	// VAL_WRITE(1, 735, 704)
	// VAL_WRITE(1, 767, 736)
	// VAL_WRITE(1, 799, 768)
	// VAL_WRITE(1, 831, 800)
	// VAL_WRITE(1, 863, 832)
	// VAL_WRITE(1, 895, 864)
	// VAL_WRITE(1, 927, 896)
	// VAL_WRITE(1, 959, 928)
	// VAL_WRITE(1, 991, 960)
	// VAL_WRITE(1, 1023, 992)

	VAL_WRITE(2, 31, 0)
	VAL_WRITE(2, 63, 32)
	VAL_WRITE(2, 95, 64)
	VAL_WRITE(2, 127, 96)
	// VAL_WRITE(2, 159, 128)
	// VAL_WRITE(2, 191, 160)
	// VAL_WRITE(2, 223, 192)
	// VAL_WRITE(2, 255, 224)
	// VAL_WRITE(2, 287, 256)
	// VAL_WRITE(2, 319, 288)
	// VAL_WRITE(2, 351, 320)
	// VAL_WRITE(2, 383, 352)
	// VAL_WRITE(2, 415, 384)
	// VAL_WRITE(2, 447, 416)
	// VAL_WRITE(2, 479, 448)
	// VAL_WRITE(2, 511, 480)
	// VAL_WRITE(2, 543, 512)
	// VAL_WRITE(2, 575, 544)
	// VAL_WRITE(2, 607, 576)
	// VAL_WRITE(2, 639, 608)
	// VAL_WRITE(2, 671, 640)
	// VAL_WRITE(2, 703, 672)
	// VAL_WRITE(2, 735, 704)
	// VAL_WRITE(2, 767, 736)
	// VAL_WRITE(2, 799, 768)
	// VAL_WRITE(2, 831, 800)
	// VAL_WRITE(2, 863, 832)
	// VAL_WRITE(2, 895, 864)
	// VAL_WRITE(2, 927, 896)
	// VAL_WRITE(2, 959, 928)
	// VAL_WRITE(2, 991, 960)
	// VAL_WRITE(2, 1023, 992)

	VAL_SWAP_1(31, 0)
	VAL_SWAP_1(63, 32)
	VAL_SWAP_1(95, 64)
	VAL_SWAP_1(127, 96)
	// VAL_SWAP_1(159, 128)
	// VAL_SWAP_1(191, 160)
	// VAL_SWAP_1(223, 192)
	// VAL_SWAP_1(255, 224)
	// VAL_SWAP_1(287, 256)
	// VAL_SWAP_1(319, 288)
	// VAL_SWAP_1(351, 320)
	// VAL_SWAP_1(383, 352)
	// VAL_SWAP_1(415, 384)
	// VAL_SWAP_1(447, 416)
	// VAL_SWAP_1(479, 448)
	// VAL_SWAP_1(511, 480)
	// VAL_SWAP_1(543, 512)
	// VAL_SWAP_1(575, 544)
	// VAL_SWAP_1(607, 576)
	// VAL_SWAP_1(639, 608)
	// VAL_SWAP_1(671, 640)
	// VAL_SWAP_1(703, 672)
	// VAL_SWAP_1(735, 704)
	// VAL_SWAP_1(767, 736)
	// VAL_SWAP_1(799, 768)
	// VAL_SWAP_1(831, 800)
	// VAL_SWAP_1(863, 832)
	// VAL_SWAP_1(895, 864)
	// VAL_SWAP_1(927, 896)
	// VAL_SWAP_1(959, 928)
	// VAL_SWAP_1(991, 960)
	// VAL_SWAP_1(1023, 992)

	VAL_SWAP_2(31, 0)
	VAL_SWAP_2(63, 32)
	VAL_SWAP_2(95, 64)
	VAL_SWAP_2(127, 96)
	// VAL_SWAP_2(159, 128)
	// VAL_SWAP_2(191, 160)
	// VAL_SWAP_2(223, 192)
	// VAL_SWAP_2(255, 224)
	// VAL_SWAP_2(287, 256)
	// VAL_SWAP_2(319, 288)
	// VAL_SWAP_2(351, 320)
	// VAL_SWAP_2(383, 352)
	// VAL_SWAP_2(415, 384)
	// VAL_SWAP_2(447, 416)
	// VAL_SWAP_2(479, 448)
	// VAL_SWAP_2(511, 480)
	// VAL_SWAP_2(543, 512)
	// VAL_SWAP_2(575, 544)
	// VAL_SWAP_2(607, 576)
	// VAL_SWAP_2(639, 608)
	// VAL_SWAP_2(671, 640)
	// VAL_SWAP_2(703, 672)
	// VAL_SWAP_2(735, 704)
	// VAL_SWAP_2(767, 736)
	// VAL_SWAP_2(799, 768)
	// VAL_SWAP_2(831, 800)
	// VAL_SWAP_2(863, 832)
	// VAL_SWAP_2(895, 864)
	// VAL_SWAP_2(927, 896)
	// VAL_SWAP_2(959, 928)
	// VAL_SWAP_2(991, 960)
	// VAL_SWAP_2(1023, 992)

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts_1)
			table_1_ts_1_write = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			if (val != 0) {
				val = ig_prsr_md.global_tstamp[31:0];
			}
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_IDX_WIDTH>, bit<16>>(reg_table_1_ts_2)
			table_1_ts_2_write = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
			if (val != 0) {
				val = ig_prsr_md.global_tstamp[47:32];
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts_1)
			table_2_ts_1_write = {
		void apply(inout bit<32> val, out bit<32> res) {
			res = val;
			if (val != 0) {
				val = ig_prsr_md.global_tstamp[31:0];
			}
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_IDX_WIDTH>, bit<16>>(reg_table_2_ts_2)
			table_2_ts_2_write = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
			if (val != 0) {
				val = ig_prsr_md.global_tstamp[47:32];
			}
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_1_ts_1)
			table_1_ts_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_prsr_md.global_tstamp[31:0] - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = ig_md.entry_ts;
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_IDX_WIDTH>, bit<16>>(reg_table_1_ts_2)
			table_1_ts_2_swap = {
		void apply(inout bit<16> val, out bit<16> res) {
			bit<16> diff = ig_prsr_md.global_tstamp[47:32] - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = ig_md.entry_ts_2;
		}
	};

	RegisterAction<bit<32>, bit<CUCKOO_IDX_WIDTH>, bit<32>>(reg_table_2_ts_1)
			table_2_ts_1_swap = {
		void apply(inout bit<32> val, out bit<32> res) {
			bit<32> diff = ig_prsr_md.global_tstamp[31:0] - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = ig_md.entry_ts;
		}
	};

	RegisterAction<bit<16>, bit<CUCKOO_IDX_WIDTH>, bit<16>>(reg_table_2_ts_2)
			table_2_ts_2_swap = {
		void apply(inout bit<16> val, out bit<16> res) {
			bit<16> diff = ig_prsr_md.global_tstamp[47:32] - val;
			if (diff > ENTRY_TIMEOUT) {
				res = 0;
			} else {
				res = val;
			}
			val = ig_md.entry_ts_2;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_incr = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient) swap_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swap_transient)
			swap_transient_clear = {
		void apply(inout bit<16> val) {
			val = 0;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient)
			swapped_transient_incr = {
		void apply(inout bit<16> val) {
			val = val |+| 1;
		}
	};

	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient)
			swapped_transient_read = {
		void apply(inout bit<16> val, out bit<16> res) {
			res = val;
		}
	};
	RegisterAction<bit<16>, bit<BLOOM_IDX_WIDTH>, bit<16>>(swapped_transient)
			swapped_transient_clear = {
		void apply(inout bit<16> val) {
			val = 0;
		}
	};

	action set_out_port(PortId_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	action set_server_reply() {
		ig_md.is_server_reply = 1;
	}

	action set_not_server_reply() {
		ig_md.is_server_reply = 0;
	}

	table is_server_reply {
		key = {
			ig_intr_md.ingress_port : exact;
		}
		actions = {
			set_server_reply;
			set_not_server_reply;
		}

		const default_action = set_not_server_reply;
		size = 2;
	}

	CRCPolynomial<bit<16>>(
		coeff	 = 0x0589,
		reversed = false,
		msb		 = false,
		extended = false,
		init	 = 0x0000,
		xor		 = 0x0001
	) poly_crc_16_dect;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x0589,
		reversed = false,
		msb		 = false,
		extended = false,
		init	 = 0x0000,
		xor		 = 0x0001
	) poly_crc_16_dect2;

	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CRC16)  hash_table_1;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_dect) hash_table_2;
	Hash<bit<CUCKOO_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_dect2) hash_table_2_recirc;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello2;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello3;

	CRCPolynomial<bit<16>>(
		coeff	 = 0x1021,
		reversed = true,
		msb		 = false,
		extended = false,
		init	 = 0xB2AA,
		xor		 = 0x0000
	) poly_crc_16_riello4;

	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello)	hash_swap;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello2) hash_swap_2;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello3) hash_swapped;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, poly_crc_16_riello4) hash_swapped_2;

	Register<bit<8>, bit<1>>(1) reg_recirc_port_cntr;

	RegisterAction<bit<8>, bit<8>, bit<8>>(reg_recirc_port_cntr) ract_recirc_port_cntr = {
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

	action recirc_port_cntr() {
		ig_md.cur_recirc_port_cntr = ract_recirc_port_cntr.execute(0);
	}

	action set_recirc_port(PortId_t port) {
		ig_tm_md.ucast_egress_port = port;
	}

	table select_recirc_port {
		key = {
			ig_md.cur_recirc_port_cntr : exact;
		}
		actions = {
			set_recirc_port;
		}
		size = 4;
		const entries = {
			0 : set_recirc_port(RECIRC_PORT_0);
			1 : set_recirc_port(RECIRC_PORT_1);
			2 : set_recirc_port(RECIRC_PORT_2);
			3 : set_recirc_port(RECIRC_PORT_3);
		}
	}

	apply {
		if (hdr.cuckoo.isValid() && hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
			// If swap pkt, get data from the swap header.
			ig_md.cur_key		= hdr.cur_swap.key;
			ig_md.cur_val		= hdr.cur_swap.val;
			ig_md.entry_ts		= hdr.cur_swap.ts;
			ig_md.entry_ts_2	= hdr.cur_swap.ts_2;
		} else {
			// Else, get data from the original headers.
			ig_md.cur_key		= hdr.kv.key;
			ig_md.cur_val		= hdr.kv.val;
			ig_md.entry_ts		= ig_prsr_md.global_tstamp[31:0];
			ig_md.entry_ts_2	= ig_prsr_md.global_tstamp[47:32];
		}

		is_server_reply.apply();

		ig_md.hash_table_1 = hash_table_1.get({ig_md.cur_key});
		ig_md.hash_table_2_r = hash_table_2_recirc.get({ig_md.cur_key});

		if (ig_md.is_server_reply == 0) {
			if (!hdr.cuckoo.isValid()) {
				hdr.cuckoo.setValid();
				hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
				hdr.cuckoo.recirc_cntr = 0;
			}
			
			if (hdr.cuckoo.op == cuckoo_ops_t.INSERT || hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
				// Insert the value in Table 1.
				// If the entry is not empty, swap the old value in Table 2.
				bool to_swap_1 = false;

				// Insert the current pkt in table 1.
				// Get the previously values.
				bit<32> table_1_ts_1 = table_1_ts_1_swap.execute(ig_md.hash_table_1);
				bit<16> table_1_ts_2 = table_1_ts_2_swap.execute(ig_md.hash_table_1);

				ig_md.table_1_key[31:0]		= swap_1_k0_31.execute(ig_md.hash_table_1);
				ig_md.table_1_key[63:32]	= swap_1_k32_63.execute(ig_md.hash_table_1);
				ig_md.table_1_key[95:64]	= swap_1_k64_95.execute(ig_md.hash_table_1);
				ig_md.table_1_key[127:96]	= swap_1_k96_127.execute(ig_md.hash_table_1);

				ig_md.table_1_val[31:0]		= swap_1_v0_31.execute(ig_md.hash_table_1);
				ig_md.table_1_val[63:32]	= swap_1_v32_63.execute(ig_md.hash_table_1);
				ig_md.table_1_val[95:64]	= swap_1_v64_95.execute(ig_md.hash_table_1);
				ig_md.table_1_val[127:96]	= swap_1_v96_127.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[159:128]	= swap_1_v128_159.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[191:160]	= swap_1_v160_191.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[223:192] 	= swap_1_v192_223.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[255:224] 	= swap_1_v224_255.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[287:256] 	= swap_1_v256_287.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[319:288] 	= swap_1_v288_319.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[351:320] 	= swap_1_v320_351.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[383:352] 	= swap_1_v352_383.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[415:384] 	= swap_1_v384_415.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[447:416] 	= swap_1_v416_447.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[479:448] 	= swap_1_v448_479.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[511:480] 	= swap_1_v480_511.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[543:512] 	= swap_1_v512_543.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[575:544] 	= swap_1_v544_575.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[607:576] 	= swap_1_v576_607.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[639:608] 	= swap_1_v608_639.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[671:640] 	= swap_1_v640_671.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[703:672] 	= swap_1_v672_703.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[735:704] 	= swap_1_v704_735.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[767:736] 	= swap_1_v736_767.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[799:768] 	= swap_1_v768_799.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[831:800] 	= swap_1_v800_831.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[863:832] 	= swap_1_v832_863.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[895:864] 	= swap_1_v864_895.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[927:896] 	= swap_1_v896_927.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[959:928]	= swap_1_v928_959.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[991:960]	= swap_1_v960_991.execute(ig_md.hash_table_1);
				// ig_md.table_1_val[1023:992]	= swap_1_v992_1023.execute(ig_md.hash_table_1);

				// Compare the previously stored values against the current pkt's.
				// If the previous stored values haven't expired and they don't match
				// the current pkt's, we will swap them to Table 2.
				if (table_1_ts_1_entry != 0 && table_1_ts_2_entry != 0) {
					// The entry hasn't yet expired, we need to compare the key.
					if (ig_md.table_1_key[63:0] != 0) {
						if (ig_md.table_1_key[127:64] != 0) {
						// if (ig_md.table_1_key[95:64] != 0) {
							to_swap_1 = true;
						}
					}
				}

				// The previous Table 1 entry was occupied and not yet expired,
				// so we'll swap it to Table 2.
				if (to_swap_1) {
					bool to_swap_2 = false;

					bit<32> table_2_ts_1 = table_2_ts_1_swap.execute(ig_md.hash_table_2);
					bit<16> table_2_ts_2 = table_2_ts_2_swap.execute(ig_md.hash_table_2);

					ig_md.table_2_key[31:0]		= swap_2_k0_31.execute(ig_md.hash_table_2);
					ig_md.table_2_key[63:32]	= swap_2_k32_63.execute(ig_md.hash_table_2);
					ig_md.table_2_key[95:64]	= swap_2_k64_95.execute(ig_md.hash_table_2);
					ig_md.table_2_key[127:96]	= swap_2_k96_127.execute(ig_md.hash_table_2);

					ig_md.table_2_val[31:0]		= swap_2_v0_31.execute(ig_md.hash_table_2_r);
					ig_md.table_2_val[63:32]	= swap_2_v32_63.execute(ig_md.hash_table_2_r);
					ig_md.table_2_val[95:64]	= swap_2_v64_95.execute(ig_md.hash_table_2_r);
					ig_md.table_2_val[127:96]	= swap_2_v96_127.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[159:128]	= swap_2_v128_159.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[191:160]	= swap_2_v160_191.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[223:192] 	= swap_2_v192_223.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[255:224] 	= swap_2_v224_255.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[287:256] 	= swap_2_v256_287.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[319:288] 	= swap_2_v288_319.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[351:320] 	= swap_2_v320_351.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[383:352] 	= swap_2_v352_383.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[415:384] 	= swap_2_v384_415.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[447:416] 	= swap_2_v416_447.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[479:448] 	= swap_2_v448_479.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[511:480] 	= swap_2_v480_511.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[543:512] 	= swap_2_v512_543.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[575:544] 	= swap_2_v544_575.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[607:576] 	= swap_2_v576_607.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[639:608] 	= swap_2_v608_639.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[671:640] 	= swap_2_v640_671.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[703:672] 	= swap_2_v672_703.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[735:704] 	= swap_2_v704_735.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[767:736] 	= swap_2_v736_767.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[799:768] 	= swap_2_v768_799.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[831:800] 	= swap_2_v800_831.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[863:832] 	= swap_2_v832_863.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[895:864] 	= swap_2_v864_895.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[927:896] 	= swap_2_v896_927.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[959:928]	= swap_2_v928_959.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[991:960]	= swap_2_v960_991.execute(ig_md.hash_table_2_r);
					// ig_md.table_2_val[1023:992]	= swap_2_v992_1023.execute(ig_md.hash_table_2_r);

					if (table_2_ts_1_entry != 0 && table_2_ts_2_entry != 0) {
						// The entry hasn't yet expired, we need to compare the key.
						if (ig_md.table_2_key[63:0] != 0) {
							if (ig_md.table_2_key[127:64] != 0) {
							// if (ig_md.table_2_key[95:64] != 0) {
								to_swap_2 = true;
							}
						}
					}

					// The previous Table 2 entry was occupied and not yet expired,
					// so we'll recirculate and swap it to Table 1.
					if (to_swap_2) {
						// Cur pkt has been sucessfully inserted.
						// The swap entry will be insert on hdr.next_swap.
						// During the bloom processing, hdr.next_swap will become hdr.cur_swap.
						ig_md.has_next_swap	= 1;
						hdr.cur_swap.key	= ig_md.table_2_key;
						hdr.cur_swap.val	= ig_md.table_2_val;
						hdr.cur_swap.ts		= table_2_ts_1;
						hdr.cur_swap.ts_2	= table_2_ts_2;
						if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
							// First (re)circulation for the current pkt.
							// Store the swap entry values and change the op to SWAP.
							// The packet will be recirculated later.
							hdr.cuckoo.op = cuckoo_ops_t.SWAP;
						} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
							// The current pkt is already a mirrored pkt.
							// Store the new swap entry values and change the op to SWAPPED.
							// The packet will be recirculated later.
							hdr.cuckoo.op		= cuckoo_ops_t.SWAPPED;
							ig_md.swapped_key	= hdr.cur_swap.key;
						}
					} else {
						// The previous Table 2 entry was expired/replaced.
						// In its place is now the cur pkt.
						if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
							// Send the current pkt to the bloom filter with op == NOP,
							// so it'll be sent out.
							hdr.cuckoo.op = cuckoo_ops_t.NOP;
						} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
							// Send the current pkt to the bloom filter with op == SWAPPED,
							// to update the transient.
							hdr.cuckoo.op = cuckoo_ops_t.SWAPPED;
						}
					}
				} else {
					// The previous Table 1 entry was expired/replaced.
					// In its place is now the cur pkt.
					if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
						// Send the current pkt to the bloom filter with op == NOP,
						// so it'll be sent out.
						hdr.cuckoo.op = cuckoo_ops_t.NOP;
					} else if (hdr.cuckoo.op == cuckoo_ops_t.SWAP) {
						// Send the current pkt to the bloom filter with op == SWAPPED,
						// to update the transient.
						hdr.cuckoo.op = cuckoo_ops_t.SWAPPED;
					}
				}
			} else if (hdr.cuckoo.op == cuckoo_ops_t.LOOKUP) {
				// Add the original ingress port.
				hdr.kv.port = (bit<16>)ig_intr_md.ingress_port;

				// Update the stored ts values with the current ts.
				bit<32> stored_ts_1_1 = table_1_ts_1_write.execute(ig_md.hash_table_1);
				bit<16> stored_ts_1_2 = table_1_ts_2_write.execute(ig_md.hash_table_1);

				if (stored_ts_1_1 != 0 && stored_ts_1_2 != 0) {
					// Read the value.
					bit<32> table_1_key_0 = read_1_k0_31.execute(ig_md.hash_table_1);
					bit<32> table_1_key_1 = read_1_k32_63.execute(ig_md.hash_table_1);
					bit<32> table_1_key_2 = read_1_k64_95.execute(ig_md.hash_table_1);
					bit<32> table_1_key_3 = read_1_k96_127.execute(ig_md.hash_table_1);

					if (table_1_key_0 == ig_md.cur_key[31:0]) {
					if (table_1_key_1 == ig_md.cur_key[63:32]) {
					if (table_1_key_2 == ig_md.cur_key[95:64]) {
					if (table_1_key_3 == ig_md.cur_key[127:96]) {
						if (hdr.kv.op == kv_ops_t.GET) {
							hdr.kv.val[31:0]	= read_1_v0_31.execute(ig_md.hash_table_1);
							hdr.kv.val[63:32]	= read_1_v32_63.execute(ig_md.hash_table_1);
							hdr.kv.val[95:64]	= read_1_v64_95.execute(ig_md.hash_table_1);
							hdr.kv.val[127:96]	= read_1_v96_127.execute(ig_md.hash_table_1);
							// hdr.kv.val[159:128]	= read_1_v128_159.execute(ig_md.hash_table_1);
							// hdr.kv.val[191:160]	= read_1_v160_191.execute(ig_md.hash_table_1);
							// hdr.kv.val[223:192]	= read_1_v192_223.execute(ig_md.hash_table_1);
							// hdr.kv.val[255:224]	= read_1_v224_255.execute(ig_md.hash_table_1);
							// hdr.kv.val[287:256]	= read_1_v256_287.execute(ig_md.hash_table_1);
							// hdr.kv.val[319:288]	= read_1_v288_319.execute(ig_md.hash_table_1);
							// hdr.kv.val[351:320]	= read_1_v320_351.execute(ig_md.hash_table_1);
							// hdr.kv.val[383:352]	= read_1_v352_383.execute(ig_md.hash_table_1);
							// hdr.kv.val[415:384]	= read_1_v384_415.execute(ig_md.hash_table_1);
							// hdr.kv.val[447:416]	= read_1_v416_447.execute(ig_md.hash_table_1);
							// hdr.kv.val[479:448]	= read_1_v448_479.execute(ig_md.hash_table_1);
							// hdr.kv.val[511:480]	= read_1_v480_511.execute(ig_md.hash_table_1);
							// hdr.kv.val[543:512]	= read_1_v512_543.execute(ig_md.hash_table_1);
							// hdr.kv.val[575:544]	= read_1_v544_575.execute(ig_md.hash_table_1);
							// hdr.kv.val[607:576]	= read_1_v576_607.execute(ig_md.hash_table_1);
							// hdr.kv.val[639:608]	= read_1_v608_639.execute(ig_md.hash_table_1);
							// hdr.kv.val[671:640]	= read_1_v640_671.execute(ig_md.hash_table_1);
							// hdr.kv.val[703:672]	= read_1_v672_703.execute(ig_md.hash_table_1);
							// hdr.kv.val[735:704]	= read_1_v704_735.execute(ig_md.hash_table_1);
							// hdr.kv.val[767:736]	= read_1_v736_767.execute(ig_md.hash_table_1);
							// hdr.kv.val[799:768]	= read_1_v768_799.execute(ig_md.hash_table_1);
							// hdr.kv.val[831:800]	= read_1_v800_831.execute(ig_md.hash_table_1);
							// hdr.kv.val[863:832]	= read_1_v832_863.execute(ig_md.hash_table_1);
							// hdr.kv.val[895:864]	= read_1_v864_895.execute(ig_md.hash_table_1);
							// hdr.kv.val[927:896]	= read_1_v896_927.execute(ig_md.hash_table_1);
							// hdr.kv.val[959:928]	= read_1_v928_959.execute(ig_md.hash_table_1);
							// hdr.kv.val[991:960]	= read_1_v960_991.execute(ig_md.hash_table_1);
							// hdr.kv.val[1023:992]	= read_1_v992_1023.execute(ig_md.hash_table_1);
						} else if (hdr.kv.op == kv_ops_t.PUT) {
							write_1_v0_31.execute(ig_md.hash_table_1);
							write_1_v32_63.execute(ig_md.hash_table_1);
							write_1_v64_95.execute(ig_md.hash_table_1);
							write_1_v96_127.execute(ig_md.hash_table_1);
							// write_1_v128_159.execute(ig_md.hash_table_1);
							// write_1_v160_191.execute(ig_md.hash_table_1);
							// write_1_v192_223.execute(ig_md.hash_table_1);
							// write_1_v224_255.execute(ig_md.hash_table_1);
							// write_1_v256_287.execute(ig_md.hash_table_1);
							// write_1_v288_319.execute(ig_md.hash_table_1);
							// write_1_v320_351.execute(ig_md.hash_table_1);
							// write_1_v352_383.execute(ig_md.hash_table_1);
							// write_1_v384_415.execute(ig_md.hash_table_1);
							// write_1_v416_447.execute(ig_md.hash_table_1);
							// write_1_v448_479.execute(ig_md.hash_table_1);
							// write_1_v480_511.execute(ig_md.hash_table_1);
							// write_1_v512_543.execute(ig_md.hash_table_1);
							// write_1_v544_575.execute(ig_md.hash_table_1);
							// write_1_v576_607.execute(ig_md.hash_table_1);
							// write_1_v608_639.execute(ig_md.hash_table_1);
							// write_1_v640_671.execute(ig_md.hash_table_1);
							// write_1_v672_703.execute(ig_md.hash_table_1);
							// write_1_v704_735.execute(ig_md.hash_table_1);
							// write_1_v736_767.execute(ig_md.hash_table_1);
							// write_1_v768_799.execute(ig_md.hash_table_1);
							// write_1_v800_831.execute(ig_md.hash_table_1);
							// write_1_v832_863.execute(ig_md.hash_table_1);
							// write_1_v864_895.execute(ig_md.hash_table_1);
							// write_1_v896_927.execute(ig_md.hash_table_1);
							// write_1_v928_959.execute(ig_md.hash_table_1);
							// write_1_v960_991.execute(ig_md.hash_table_1);
							// write_1_v992_1023.execute(ig_md.hash_table_1);
						}
					}
					}
					}
					}
					hdr.cuckoo.op = cuckoo_ops_t.NOP;
				} else {
					// Table 2 Lookup.
					ig_md.hash_table_2 = hash_table_2.get({ig_md.cur_key});

					// Update the stored ts values with the current ts.
					bit<32> stored_ts_2_1 = table_2_ts_1_write.execute(ig_md.hash_table_2);
					bit<16> stored_ts_2_2 = table_2_ts_2_write.execute(ig_md.hash_table_2);

					if (stored_ts_2_1 != 0 && stored_ts_2_2 != 0) {
						// Read the value.
						bit<32> table_2_key_0 = read_2_k0_31.execute(ig_md.hash_table_2);
						bit<32> table_2_key_1 = read_2_k32_63.execute(ig_md.hash_table_2);
						bit<32> table_2_key_2 = read_2_k64_95.execute(ig_md.hash_table_2);
						bit<32> table_2_key_3 = read_2_k96_127.execute(ig_md.hash_table_2);

						if (table_2_key_0 == ig_md.cur_key[31:0]) {
						if (table_2_key_1 == ig_md.cur_key[63:32]) {
						if (table_2_key_2 == ig_md.cur_key[95:64]) {
						if (table_2_key_3 == ig_md.cur_key[127:96]) {
							if (hdr.kv.op == kv_ops_t.GET) {
								hdr.kv.val[31:0]	= read_2_v0_31.execute(ig_md.hash_table_2);
								hdr.kv.val[63:32]	= read_2_v32_63.execute(ig_md.hash_table_2);
								hdr.kv.val[95:64]	= read_2_v64_95.execute(ig_md.hash_table_2);
								hdr.kv.val[127:96]	= read_2_v96_127.execute(ig_md.hash_table_2);
								// hdr.kv.val[159:128]	= read_2_v128_159.execute(ig_md.hash_table_2);
								// hdr.kv.val[191:160]	= read_2_v160_191.execute(ig_md.hash_table_2);
								// hdr.kv.val[223:192]	= read_2_v192_223.execute(ig_md.hash_table_2);
								// hdr.kv.val[255:224]	= read_2_v224_255.execute(ig_md.hash_table_2);
								// hdr.kv.val[287:256]	= read_2_v256_287.execute(ig_md.hash_table_2);
								// hdr.kv.val[319:288]	= read_2_v288_319.execute(ig_md.hash_table_2);
								// hdr.kv.val[351:320]	= read_2_v320_351.execute(ig_md.hash_table_2);
								// hdr.kv.val[383:352]	= read_2_v352_383.execute(ig_md.hash_table_2);
								// hdr.kv.val[415:384]	= read_2_v384_415.execute(ig_md.hash_table_2);
								// hdr.kv.val[447:416]	= read_2_v416_447.execute(ig_md.hash_table_2);
								// hdr.kv.val[479:448]	= read_2_v448_479.execute(ig_md.hash_table_2);
								// hdr.kv.val[511:480]	= read_2_v480_511.execute(ig_md.hash_table_2);
								// hdr.kv.val[543:512]	= read_2_v512_543.execute(ig_md.hash_table_2);
								// hdr.kv.val[575:544]	= read_2_v544_575.execute(ig_md.hash_table_2);
								// hdr.kv.val[607:576]	= read_2_v576_607.execute(ig_md.hash_table_2);
								// hdr.kv.val[639:608]	= read_2_v608_639.execute(ig_md.hash_table_2);
								// hdr.kv.val[671:640]	= read_2_v640_671.execute(ig_md.hash_table_2);
								// hdr.kv.val[703:672]	= read_2_v672_703.execute(ig_md.hash_table_2);
								// hdr.kv.val[735:704]	= read_2_v704_735.execute(ig_md.hash_table_2);
								// hdr.kv.val[767:736]	= read_2_v736_767.execute(ig_md.hash_table_2);
								// hdr.kv.val[799:768]	= read_2_v768_799.execute(ig_md.hash_table_2);
								// hdr.kv.val[831:800]	= read_2_v800_831.execute(ig_md.hash_table_2);
								// hdr.kv.val[863:832]	= read_2_v832_863.execute(ig_md.hash_table_2);
								// hdr.kv.val[895:864]	= read_2_v864_895.execute(ig_md.hash_table_2);
								// hdr.kv.val[927:896]	= read_2_v896_927.execute(ig_md.hash_table_2);
								// hdr.kv.val[959:928]	= read_2_v928_959.execute(ig_md.hash_table_2);
								// hdr.kv.val[991:960]	= read_2_v960_991.execute(ig_md.hash_table_2);
								// hdr.kv.val[1023:992]	= read_2_v992_1023.execute(ig_md.hash_table_2);
							} else if (hdr.kv.op == kv_ops_t.PUT) {
								write_2_v0_31.execute(ig_md.hash_table_2);
								write_2_v32_63.execute(ig_md.hash_table_2);
								write_2_v64_95.execute(ig_md.hash_table_2);
								write_2_v96_127.execute(ig_md.hash_table_2);
								// write_2_v128_159.execute(ig_md.hash_table_2);
								// write_2_v160_191.execute(ig_md.hash_table_2);
								// write_2_v192_223.execute(ig_md.hash_table_2);
								// write_2_v224_255.execute(ig_md.hash_table_2);
								// write_2_v256_287.execute(ig_md.hash_table_2);
								// write_2_v288_319.execute(ig_md.hash_table_2);
								// write_2_v320_351.execute(ig_md.hash_table_2);
								// write_2_v352_383.execute(ig_md.hash_table_2);
								// write_2_v384_415.execute(ig_md.hash_table_2);
								// write_2_v416_447.execute(ig_md.hash_table_2);
								// write_2_v448_479.execute(ig_md.hash_table_2);
								// write_2_v480_511.execute(ig_md.hash_table_2);
								// write_2_v512_543.execute(ig_md.hash_table_2);
								// write_2_v544_575.execute(ig_md.hash_table_2);
								// write_2_v576_607.execute(ig_md.hash_table_2);
								// write_2_v608_639.execute(ig_md.hash_table_2);
								// write_2_v640_671.execute(ig_md.hash_table_2);
								// write_2_v672_703.execute(ig_md.hash_table_2);
								// write_2_v704_735.execute(ig_md.hash_table_2);
								// write_2_v736_767.execute(ig_md.hash_table_2);
								// write_2_v768_799.execute(ig_md.hash_table_2);
								// write_2_v800_831.execute(ig_md.hash_table_2);
								// write_2_v832_863.execute(ig_md.hash_table_2);
								// write_2_v864_895.execute(ig_md.hash_table_2);
								// write_2_v896_927.execute(ig_md.hash_table_2);
								// write_2_v928_959.execute(ig_md.hash_table_2);
								// write_2_v960_991.execute(ig_md.hash_table_2);
								// write_2_v992_1023.execute(ig_md.hash_table_2);
							}
						}
						}
						}
						}
							hdr.cuckoo.op	= cuckoo_ops_t.NOP;
						} else {
							// Lookup failed, recirculate the packet to insert it in the table.
							hdr.cuckoo.op = cuckoo_ops_t.INSERT;
					}
				}
			}

			if (ig_md.has_next_swap == 1 || hdr.cuckoo.op == cuckoo_ops_t.INSERT || hdr.cuckoo.op == cuckoo_ops_t.SWAPPED) {
				// Handlers for transient states.
				bit<16> swap_transient_val		= 0;
				bit<16> swapped_transient_val	= 0;

				if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
					bit<BLOOM_IDX_WIDTH> swap_transient_idx = hash_swap.get({ig_md.cur_key});

					// If we've reached MAX_LOOPS_INSERT recirculations,
					// then assume that the previous swap pkt is lost and reset the transients.
					if (hdr.cuckoo.recirc_cntr == MAX_LOOPS) {
						swap_transient_clear.execute(swap_transient_idx);
						swapped_transient_clear.execute(swap_transient_idx);
					} else {
						// Read the current transient values.
						swap_transient_val		= swap_transient_read.execute(swap_transient_idx);
						swapped_transient_val	= swapped_transient_read.execute(swap_transient_idx);
					}
				} else if (ig_md.has_next_swap == 0 && hdr.cuckoo.op == cuckoo_ops_t.SWAPPED) {
					// If op == SWAPPED and the pkt doesn't have a swap entry,
					// increment the swapped transient value.
					swapped_transient_incr.execute(hash_swapped.get({hdr.cur_swap.key}));
					hdr.cuckoo.op = cuckoo_ops_t.NOP;
				} else if (ig_md.has_next_swap == 1) {
					// The current pkt is a swap pkt. It will be recirculated back.
					// Set the cur_swap hdr with the next_swap hdr values.
					hdr.cuckoo.op = cuckoo_ops_t.SWAP;

					// Increment the swap transient.
					swap_transient_incr.execute(hash_swap_2.get({hdr.cur_swap.key}));

					// Additionally, if op == SWAPPED, increment the swapped transient.
					if (hdr.cuckoo.op == cuckoo_ops_t.SWAPPED) {
						swapped_transient_incr.execute(hash_swapped_2.get({ig_md.swapped_key}));
					}
				}

				if (hdr.cuckoo.op == cuckoo_ops_t.INSERT) {
					// If op == INSERT, the pkt will recirculate back to the cuckoo pipe.
					if (swap_transient_val != swapped_transient_val) {
						// However, if another pkt matching the same bloom idx is already
						// recirculating, the current pkt will be sent back as a LOOKUP.
						hdr.cuckoo.op = cuckoo_ops_t.LOOKUP;
					}
				}
			}

			if (hdr.cuckoo.op == cuckoo_ops_t.NOP) {
				set_out_port((bit<9>)hdr.kv.port);
			} else if (ig_md.has_next_swap == 1 && hdr.cuckoo.recirc_cntr == MAX_LOOPS) {
				set_out_port(KVS_SERVER_PORT);
			} else {
				hdr.cuckoo.recirc_cntr = hdr.cuckoo.recirc_cntr + 1;
				recirc_port_cntr();
				select_recirc_port.apply();
			}
		} else {
			set_out_port((bit<9>)hdr.kv.port);
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
	apply {}
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
