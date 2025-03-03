#ifndef _BLOOM_
#define _BLOOM_

control c_bloom(inout header_t hdr, out bit<1> bloom_result) {
	Register<bit<1>, _>(BLOOM_ENTRIES) reg_bloom_0;
	Register<bit<1>, _>(BLOOM_ENTRIES) reg_bloom_1;
	Register<bit<1>, _>(BLOOM_ENTRIES) reg_bloom_2;

	CRCPolynomial<bit<32>>(coeff	= 0x1EDC6F41,
						   reversed = true,
						   msb		= false,
						   extended = false,
						   init		= 0x00000000,
						   xor		= 0xFFFFFFFF) crc32_c;

	CRCPolynomial<bit<32>>(coeff	= 0xA833982B,
						   reversed = true,
						   msb		= false,
						   extended = false,
						   init		= 0x00000000,
						   xor		= 0xFFFFFFFF) crc32_d;

	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CRC32)			hash_crc32;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, crc32_c)	hash_crc32_c;
	Hash<bit<BLOOM_IDX_WIDTH>>(HashAlgorithm_t.CUSTOM, crc32_d)	hash_crc32_d;

	bit<BLOOM_IDX_WIDTH> hash_bloom_0;
	bit<BLOOM_IDX_WIDTH> hash_bloom_1;
	bit<BLOOM_IDX_WIDTH> hash_bloom_2;

	bit<1> val_bloom_0;
	bit<1> val_bloom_1;
	bit<1> val_bloom_2;

	RegisterAction<_, bit<BLOOM_IDX_WIDTH>, bit<1>>(reg_bloom_0) ract_bloom_0_update = {
		void apply(inout bit<1> val, out bit<1> res) {
			res = val;
			val = 1;
		}
	};

	RegisterAction<_, bit<BLOOM_IDX_WIDTH>, bit<1>>(reg_bloom_1) ract_bloom_1_update = {
		void apply(inout bit<1> val, out bit<1> res) {
			res = val;
			val = 1;
		}
	};

	RegisterAction<_, bit<BLOOM_IDX_WIDTH>, bit<1>>(reg_bloom_2) ract_bloom_2_update = {
		void apply(inout bit<1> val, out bit<1> res) {
			res = val;
			val = 1;
		}
	};

	action hash_calc_bloom_0() {
		hash_bloom_0 = (bit<BLOOM_IDX_WIDTH>)hash_crc32.get({hdr.netcache.key})[BLOOM_IDX_WIDTH-1:0];
	}

	action hash_calc_bloom_1() {
		hash_bloom_1 = (bit<BLOOM_IDX_WIDTH>)hash_crc32_c.get({hdr.netcache.key})[BLOOM_IDX_WIDTH-1:0];
	}

	action hash_calc_bloom_2() {
		hash_bloom_2 = (bit<BLOOM_IDX_WIDTH>)hash_crc32_d.get({hdr.netcache.key})[BLOOM_IDX_WIDTH-1:0];
	}

	action bloom_0_update() {
		val_bloom_0 = ract_bloom_0_update.execute(hash_bloom_0);
	}

	action bloom_1_update() {
		val_bloom_1 = ract_bloom_1_update.execute(hash_bloom_1);
	}

	action bloom_2_update() {
		val_bloom_2 = ract_bloom_2_update.execute(hash_bloom_2);
	}

	apply {
		hash_calc_bloom_0();
		hash_calc_bloom_1();
		hash_calc_bloom_2();

		bloom_0_update();
		bloom_1_update();
		bloom_2_update();

		// If all the obtained values are 1, then the current key has already been reported
		// to the controller with high probability.
		if (val_bloom_0 == 1 && val_bloom_1 == 1 && val_bloom_2 == 1) {
			bloom_result = 1;
		}
	}
}

#endif
