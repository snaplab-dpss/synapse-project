#ifndef _CM_
#define _CM_

control c_cm(inout header_t hdr, out bit<32> cm_result) {

	Register<bit<32>, bit<NC_KEY_WIDTH>>(SKETCH_ENTRIES) reg_cm_0;
	Register<bit<32>, bit<NC_KEY_WIDTH>>(SKETCH_ENTRIES) reg_cm_1;
	Register<bit<32>, bit<NC_KEY_WIDTH>>(SKETCH_ENTRIES) reg_cm_2;
	Register<bit<32>, bit<NC_KEY_WIDTH>>(SKETCH_ENTRIES) reg_cm_3;

	CRCPolynomial<bit<16>>(coeff    = 0x0589,
						   reversed = false,
                           msb      = false,
                           extended = false,
                           init     = 16w0x0001,
                           xor      = 16w0x0001) crc16_dect;
	
	CRCPolynomial<bit<16>>(coeff    = 0x3D65,
						   reversed = true,
						   msb      = false,
						   extended = false,
						   init     = 16w0xFFFF,
						   xor      = 16w0xFFFF) crc16_dnp;

	CRCPolynomial<bit<16>>(coeff    = 0x1021,
						   reversed = false,
						   msb      = false,
						   extended = false,
						   init     = 16w0x0000,
						   xor      = 16w0xFFFF) crc16_genibus;

	Hash<bit<16>>(HashAlgorithm_t.CRC16)				 hash_crc16;
	Hash<bit<16>>(HashAlgorithm_t.CUSTOM, crc16_dect)	 hash_crc16_dect;
	Hash<bit<16>>(HashAlgorithm_t.CUSTOM, crc16_dnp)	 hash_crc16_dnp;
	Hash<bit<16>>(HashAlgorithm_t.CUSTOM, crc16_genibus) hash_crc16_genibus;

	bit<16> hash_cm_0;
	bit<16> hash_cm_1;
	bit<16> hash_cm_2;
	bit<16> hash_cm_3;

	bit<32> val_cm_0;
	bit<32> val_cm_1;
	bit<32> val_cm_2;
	bit<32> val_cm_3;

	RegisterAction<_, bit<16>, bit<32>>(reg_cm_0) ract_cm_0_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			val = val + 1;
			res = val;
		}
	};

	RegisterAction<_, bit<16>, bit<32>>(reg_cm_1) ract_cm_1_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			val = val + 1;
			res = val;
		}
	};

	RegisterAction<_, bit<16>, bit<32>>(reg_cm_2) ract_cm_2_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			val = val + 1;
			res = val;
		}
	};

	RegisterAction<_, bit<16>, bit<32>>(reg_cm_3) ract_cm_3_update = {
		void apply(inout bit<32> val, out bit<32> res) {
			val = val + 1;
			res = val;
		}
	};

	action hash_calc_cm_0() {
		hash_cm_0 = hash_crc16.get({hdr.netcache.key});
	}

	action hash_calc_cm_1() {
		hash_cm_1 = hash_crc16_dect.get({hdr.netcache.key});
	}

	action hash_calc_cm_2() {
		hash_cm_2 = hash_crc16_dnp.get({hdr.netcache.key});
	}

	action hash_calc_cm_3() {
		hash_cm_3 = hash_crc16_genibus.get({hdr.netcache.key});
	}

	action cm_0_update() {
		val_cm_0 = ract_cm_0_update.execute(hash_cm_0);
	}

	action cm_1_update() {
       val_cm_1 = ract_cm_1_update.execute(hash_cm_1);
	}

	action cm_2_update() {
       val_cm_2 = ract_cm_2_update.execute(hash_cm_2);
	}

	action cm_3_update() {
       val_cm_3 = ract_cm_3_update.execute(hash_cm_3);
	}

	apply {
		hash_calc_cm_0();
		hash_calc_cm_1();
		hash_calc_cm_2();
		hash_calc_cm_3();

		cm_0_update();
		cm_1_update();
		cm_2_update();
		cm_3_update();

		// CM sketch: compare all obtained values and output the minimum.

		// Compare the current value on all registers and identify the smallest.
        cm_result = val_cm_0;
		cm_result = min(cm_result, val_cm_1);
		cm_result = min(cm_result, val_cm_2);
		cm_result = min(cm_result, val_cm_3);
	}
}

#endif
