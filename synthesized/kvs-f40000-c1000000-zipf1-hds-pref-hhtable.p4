#include <core.p4>

#if __TARGET_TOFINO__ == 2
  #include <t2na.p4>
  #define CPU_PCIE_PORT 0
  #define RECIRCULATION_PORT_0 6
  #define RECIRCULATION_PORT_1 128
  #define RECIRCULATION_PORT_2 256
  #define RECIRCULATION_PORT_3 384
#else
  #include <tna.p4>
  #define CPU_PCIE_PORT 192
  #define RECIRCULATION_PORT_0 68
  #define RECIRCULATION_PORT_1 196
#endif

#define bswap32(x) (x[7:0] ++ x[15:8] ++ x[23:16] ++ x[31:24])
#define bswap16(x) (x[7:0] ++ x[15:8])

const bit<16> CUCKOO_CODE_PATH = 0xffff;

enum bit<8> cuckoo_ops_t {
  LOOKUP  = 0x00,
  UPDATE  = 0x01,
  INSERT  = 0x02,
  SWAP    = 0x03,
  DONE    = 0x04,
}

enum bit<2> fwd_op_t {
  FORWARD_NF_DEV  = 0,
  FORWARD_TO_CPU  = 1,
  RECIRCULATE     = 2,
  DROP            = 3,
}

header cpu_h {
  bit<16> code_path;                  // Written by the data plane
  bit<16> egress_dev;                 // Written by the control plane
  bit<8> trigger_dataplane_execution; // Written by the control plane

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;

};

header cuckoo_h {
  bit<8>  op;
  bit<8>  recirc_cntr;
  bit<32> ts;
  bit<32> key;
  bit<32> val;
  bit<8>  old_op;
  bit<32> old_key;
}

header hdr0_h {
  bit<48> data0;
  bit<48> data1;
  bit<16> data2;
}
header hdr1_h {
  bit<72> data0;
  bit<24> data1;
  bit<32> data2;
  bit<32> data3;
}
header hdr2_h {
  bit<16> data0;
  bit<16> data1;
  bit<32> data2;
}
header hdr3_h {
  bit<8> data0;
  bit<32> data1;
  bit<32> data2;
  bit<8> data3;
  bit<16> data4;
}
header hh_table_1073923800_digest_hdr {
  bit<32> data0;
}



struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;
  hdr3_h hdr3;

}

struct synapse_ingress_metadata_t {
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> time;

}

struct synapse_egress_headers_t {
  cpu_h cpu;
  recirc_h recirc;

}

struct synapse_egress_metadata_t {

}

parser TofinoIngressParser(
  packet_in pkt,
  out ingress_intrinsic_metadata_t ig_intr_md
) {
  state start {
    pkt.extract(ig_intr_md);
    transition select(ig_intr_md.resubmit_flag) {
      1: parse_resubmit;
      0: parse_port_metadata;
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
  out synapse_ingress_headers_t hdr,
  out synapse_ingress_metadata_t meta,
  out ingress_intrinsic_metadata_t ig_intr_md
) {
  TofinoIngressParser() tofino_parser;
  
  /* This is a mandatory state, required by Tofino Architecture */
  state start {
    tofino_parser.apply(pkt, ig_intr_md);

    meta.ingress_port[8:0] = ig_intr_md.ingress_port;
    meta.dev = 0;
    meta.time = ig_intr_md.ingress_mac_tstamp[47:16];

    transition select(ig_intr_md.ingress_port) {
      CPU_PCIE_PORT: parse_cpu;
      RECIRCULATION_PORT_0: parse_recirc;
      RECIRCULATION_PORT_1: parse_recirc;
#if __TARGET_TOFINO__ == 2
      RECIRCULATION_PORT_2: parse_recirc;
      RECIRCULATION_PORT_3: parse_recirc;
#endif
      default: parser_init;
    }
  }

  state parse_cpu {
    pkt.extract(hdr.cpu);
    transition accept;
  }

  state parse_recirc {
    pkt.extract(hdr.recirc);
    transition select(hdr.recirc.code_path) {
      CUCKOO_CODE_PATH: parse_cuckoo;
      default: parser_init;
    }
  }

  state parse_cuckoo {
    pkt.extract(hdr.cuckoo);
    transition parser_init;
  }

  state parser_init {
    pkt.extract(hdr.hdr0);
    transition parser_6;
  }
  state parser_6 {
    transition parser_6_0;
  }
  state parser_6_0 {
    transition select (hdr.hdr0.data2) {
      16w0x0800: parser_7;
      default: parser_66;
    }
  }
  state parser_7 {
    pkt.extract(hdr.hdr1);
    transition parser_8;
  }
  state parser_66 {
    transition reject;
  }
  state parser_8 {
    transition parser_8_0;
  }
  state parser_8_0 {
    transition select (hdr.hdr1.data1[23:16]) {
      8w0x11: parser_9;
      default: parser_64;
    }
  }
  state parser_9 {
    pkt.extract(hdr.hdr2);
    transition parser_10;
  }
  state parser_64 {
    transition reject;
  }
  state parser_10 {
    transition parser_10_0;
  }
  state parser_10_0 {
    transition select (hdr.hdr2.data0) {
      16w0x029e: parser_11;
      default: parser_10_1;
    }
  }
  state parser_10_1 {
    transition select (hdr.hdr2.data1) {
      16w0x029e: parser_11;
      default: parser_61;
    }
  }
  state parser_11 {
    pkt.extract(hdr.hdr3);
    transition parser_57;
  }
  state parser_61 {
    transition reject;
  }
  state parser_57 {
    transition accept;
  }

}



control Ingress(
  inout synapse_ingress_headers_t hdr,
  inout synapse_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_t ig_intr_md,
  in    ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
  inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
  inout ingress_intrinsic_metadata_for_tm_t ig_tm_md
) {
  action drop() {
    ig_dprsr_md.drop_ctl = 1;
  }
  
  action fwd(bit<16> port) {
    ig_tm_md.ucast_egress_port = port[8:0];
  }

  action fwd_to_cpu() {
    hdr.recirc.setInvalid();
    hdr.cuckoo.setInvalid();
    fwd(CPU_PCIE_PORT);
  }

  action fwd_nf_dev(bit<16> port) {
    hdr.cpu.setInvalid();
    hdr.recirc.setInvalid();
    hdr.cuckoo.setInvalid();
    fwd(port);
  }

  action set_ingress_dev(bit<32> nf_dev) {
    meta.dev = nf_dev;
  }

  action set_ingress_dev_from_recirculation() {
    meta.ingress_port = hdr.recirc.ingress_port;
    meta.dev = hdr.recirc.dev;
  }

  table ingress_port_to_nf_dev {
    key = {
      meta.ingress_port: exact;
    }
    actions = {
      set_ingress_dev;
      set_ingress_dev_from_recirculation;
    }

    size = 64;
  }

  fwd_op_t fwd_op = fwd_op_t.FORWARD_NF_DEV;
  bit<32> nf_dev = 0;
  table forwarding_tbl {
    key = {
      fwd_op: exact;
      nf_dev: ternary;
      meta.ingress_port: ternary;
    }

    actions = {
      fwd;
      fwd_nf_dev;
      fwd_to_cpu;
      drop;
    }

    size = 128;

    const default_action = drop();
  }

  action swap(inout bit<8> a, inout bit<8> b) {
    bit<8> tmp = a;
    a = b;
    b = tmp;
  }

  action build_cpu_hdr(bit<16> code_path) {
    hdr.cpu.setValid();
    hdr.cpu.code_path = code_path;
    fwd(CPU_PCIE_PORT);
  }

  action build_recirc_hdr(bit<16> code_path) {
    hdr.recirc.setValid();
    hdr.recirc.ingress_port = meta.ingress_port;
    hdr.recirc.dev = meta.dev;
    hdr.recirc.code_path = code_path;
  }

  action build_cuckoo_hdr(bit<32> key, bit<32> val) {
		hdr.cuckoo.setValid();
		hdr.cuckoo.recirc_cntr = 0;
		hdr.cuckoo.ts = meta.time;
		hdr.cuckoo.key = key;
		hdr.cuckoo.val = val;
	}

  bit<32> hh_table_1073923800_table_13_get_value_param0 = 32w0;
  action hh_table_1073923800_table_13_get_value(bit<32> _hh_table_1073923800_table_13_get_value_param0) {
    hh_table_1073923800_table_13_get_value_param0 = _hh_table_1073923800_table_13_get_value_param0;
  }

  bit<32> hh_table_1073923800_table_13_key0 = 32w0;
  table hh_table_1073923800_table_13 {
    key = {
      hh_table_1073923800_table_13_key0: exact;
    }
    actions = {
      hh_table_1073923800_table_13_get_value;
    }
    size = 9103;
    idle_timeout = true;
  }

  Register<bit<32>,_>(8192, 0) hh_table_1073923800_cached_counters;
  RegisterAction<bit<32>, bit<32>, void>(hh_table_1073923800_cached_counters) hh_table_1073923800_cached_counters_inc_383 = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1073923800_hash_calc_0;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1073923800_hash_calc_1;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1073923800_hash_calc_2;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1073923800_hash_calc_3;

  Register<bit<32>,_>(1024, 0) hh_table_1073923800_cms_row_0;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1073923800_cms_row_0) hh_table_1073923800_cms_row_0_inc_and_read_383 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1024, 0) hh_table_1073923800_cms_row_1;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1073923800_cms_row_1) hh_table_1073923800_cms_row_1_inc_and_read_383 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1024, 0) hh_table_1073923800_cms_row_2;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1073923800_cms_row_2) hh_table_1073923800_cms_row_2_inc_and_read_383 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1024, 0) hh_table_1073923800_cms_row_3;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1073923800_cms_row_3) hh_table_1073923800_cms_row_3_inc_and_read_383 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1, 0) hh_table_1073923800_threshold;
  bit<32> hh_table_1073923800_threshold_diff_383_cmp;
  RegisterAction<bit<32>, bit<1>, bit<32>>(hh_table_1073923800_threshold) hh_table_1073923800_threshold_diff_383 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = hh_table_1073923800_threshold_diff_383_cmp - value;
    }
  };

  bit<10> hh_table_1073923800_hash_calc_0_value;
  action hh_table_1073923800_hash_calc_0_calc() {
    hh_table_1073923800_hash_calc_0_value = hh_table_1073923800_hash_calc_0.get({
      hh_table_1073923800_table_13_key0,
      32w0xfbc31fc7
    });
  }
  bit<10> hh_table_1073923800_hash_calc_1_value;
  action hh_table_1073923800_hash_calc_1_calc() {
    hh_table_1073923800_hash_calc_1_value = hh_table_1073923800_hash_calc_1.get({
      hh_table_1073923800_table_13_key0,
      32w0x2681580b
    });
  }
  bit<10> hh_table_1073923800_hash_calc_2_value;
  action hh_table_1073923800_hash_calc_2_calc() {
    hh_table_1073923800_hash_calc_2_value = hh_table_1073923800_hash_calc_2.get({
      hh_table_1073923800_table_13_key0,
      32w0x486d7e2f
    });
  }
  bit<10> hh_table_1073923800_hash_calc_3_value;
  action hh_table_1073923800_hash_calc_3_calc() {
    hh_table_1073923800_hash_calc_3_value = hh_table_1073923800_hash_calc_3.get({
      hh_table_1073923800_table_13_key0,
      32w0x1f3a2b4d
    });
  }
  bit<32> hh_table_1073923800_cms_row_0_value;
  action hh_table_1073923800_cms_row_0_inc_and_read_383_execute() {
    hh_table_1073923800_cms_row_0_value = hh_table_1073923800_cms_row_0_inc_and_read_383.execute(hh_table_1073923800_hash_calc_0_value);
  }
  bit<32> hh_table_1073923800_cms_row_1_value;
  action hh_table_1073923800_cms_row_1_inc_and_read_383_execute() {
    hh_table_1073923800_cms_row_1_value = hh_table_1073923800_cms_row_1_inc_and_read_383.execute(hh_table_1073923800_hash_calc_1_value);
  }
  bit<32> hh_table_1073923800_cms_row_2_value;
  action hh_table_1073923800_cms_row_2_inc_and_read_383_execute() {
    hh_table_1073923800_cms_row_2_value = hh_table_1073923800_cms_row_2_inc_and_read_383.execute(hh_table_1073923800_hash_calc_2_value);
  }
  bit<32> hh_table_1073923800_cms_row_3_value;
  action hh_table_1073923800_cms_row_3_inc_and_read_383_execute() {
    hh_table_1073923800_cms_row_3_value = hh_table_1073923800_cms_row_3_inc_and_read_383.execute(hh_table_1073923800_hash_calc_3_value);
  }
  Register<bit<32>,_>(8192, 0) vector_register_1073954840_0;

  RegisterAction<bit<32>, bit<32>, bit<32>>(vector_register_1073954840_0) vector_register_1073954840_0_read_2662 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = value;
    }
  };



  RegisterAction<bit<32>, bit<32>, void>(vector_register_1073954840_0) vector_register_1073954840_0_write_3378 = {
    void apply(inout bit<32> value) {
      value = hdr.hdr3.data2;
    }
  };


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {

    } else {
      // EP node  1:Ignore
      // BDD node 4:expire_items_single_map(chain:(w64 1073971976), vector:(w64 1073937624), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1073923800))
      // EP node  6:ParserExtraction
      // BDD node 5:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 14), chunk:(w64 1074039328)[ -> (w64 1073760160)])
      if(hdr.hdr0.isValid()) {
        // EP node  17:ParserCondition
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  18:Then
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  33:ParserExtraction
        // BDD node 7:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 20), chunk:(w64 1074040064)[ -> (w64 1073760416)])
        if(hdr.hdr1.isValid()) {
          // EP node  61:ParserCondition
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  62:Then
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  86:ParserExtraction
          // BDD node 9:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 8), chunk:(w64 1074040720)[ -> (w64 1073760672)])
          if(hdr.hdr2.isValid()) {
            // EP node  131:ParserCondition
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  132:Then
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  192:ParserExtraction
            // BDD node 11:packet_borrow_next_chunk(p:(w64 1074028624), length:(w32 12), chunk:(w64 1074041464)[ -> (w64 1073760928)])
            if(hdr.hdr3.isValid()) {
              // EP node  269:If
              // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
              if ((16w0x0000) != (meta.dev[15:0])) {
                // EP node  270:Then
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  383:HHTableRead
                // BDD node 13:map_get(map:(w64 1073923800), key:(w64 1073760929)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value_out:(w64 1074038640)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
                hh_table_1073923800_table_13_key0 = hdr.hdr3.data1;
                bool hit0 = hh_table_1073923800_table_13.apply().hit;
                if (hit0) {
                  hh_table_1073923800_cached_counters_inc_383.execute(hh_table_1073923800_table_13_get_value_param0);
                } else {
                  hh_table_1073923800_hash_calc_0_calc();
                  hh_table_1073923800_hash_calc_1_calc();
                  hh_table_1073923800_hash_calc_2_calc();
                  hh_table_1073923800_hash_calc_3_calc();
                  hh_table_1073923800_cms_row_0_inc_and_read_383_execute();
                  hh_table_1073923800_cms_row_1_inc_and_read_383_execute();
                  hh_table_1073923800_cms_row_2_inc_and_read_383_execute();
                  hh_table_1073923800_cms_row_3_inc_and_read_383_execute();
                  bit<32> hh_table_1073923800_cms_min = hh_table_1073923800_cms_row_0_value;
                  hh_table_1073923800_cms_min = min(hh_table_1073923800_cms_min, hh_table_1073923800_cms_row_1_value);
                  hh_table_1073923800_cms_min = min(hh_table_1073923800_cms_min, hh_table_1073923800_cms_row_2_value);
                  hh_table_1073923800_cms_min = min(hh_table_1073923800_cms_min, hh_table_1073923800_cms_row_3_value);
                  hh_table_1073923800_threshold_diff_383_cmp = hh_table_1073923800_cms_min;
                  bit<32> hh_table_1073923800_threshold_diff = hh_table_1073923800_threshold_diff_383.execute(0);
                  if (hh_table_1073923800_threshold_diff_383_cmp[31:31] == 0) {
                    ig_dprsr_md.digest_type = 1;
                  }
                }
                // EP node  458:If
                // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                if (!hit0) {
                  // EP node  459:Then
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  536:If
                  // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                  if ((8w0x01) == (hdr.hdr3.data0)) {
                    // EP node  537:Then
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  1057:HHTableOutOfBandUpdate
                    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                    // EP node  1184:ModifyHeader
                    // BDD node 18:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 1) DEVICE) (Concat w88 (Read w8 (w32 0) DEVICE) (ReadLSB w80 (w32 768) packet_chunks)))])
                    hdr.hdr3.data4[15:8] = meta.dev[7:0];
                    hdr.hdr3.data4[7:0] = meta.dev[15:8];
                    // EP node  1551:Forward
                    // BDD node 22:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                  } else {
                    // EP node  538:Else
                    // BDD node 15:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  651:ModifyHeader
                    // BDD node 33:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 1) DEVICE) (Concat w88 (Read w8 (w32 0) DEVICE) (ReadLSB w80 (w32 768) packet_chunks)))])
                    hdr.hdr3.data4[15:8] = meta.dev[7:0];
                    hdr.hdr3.data4[7:0] = meta.dev[15:8];
                    // EP node  985:Forward
                    // BDD node 37:FORWARD
                    nf_dev[15:0] = 16w0x0000;
                  }
                } else {
                  // EP node  460:Else
                  // BDD node 14:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                  // EP node  2092:Ignore
                  // BDD node 38:dchain_rejuvenate_index(chain:(w64 1073971976), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                  // EP node  2662:VectorRegisterLookup
                  // BDD node 39:vector_borrow(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), val_out:(w64 1074041936)[ -> (w64 1073968736)])
                  bit<32> vector_reg_value0 = vector_register_1073954840_0_read_2662.execute(hh_table_1073923800_table_13_get_value_param0);
                  // EP node  3054:If
                  // BDD node 40:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                  if ((8w0x01) == (hdr.hdr3.data0)) {
                    // EP node  3055:Then
                    // BDD node 40:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  3378:VectorRegisterUpdate
                    // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
                    vector_register_1073954840_0_write_3378.execute(hh_table_1073923800_table_13_get_value_param0);
                    // EP node  3769:ModifyHeader
                    // BDD node 42:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (ReadLSB w72 (w32 768) packet_chunks))))])
                    hdr.hdr3.data3 = 8w0x01;
                    // EP node  4058:ModifyHeader
                    // BDD node 43:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760672)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                    swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                    swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                    // EP node  4359:ModifyHeader
                    // BDD node 44:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760416)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                    swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                    swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                    swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                    swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                    // EP node  4672:ModifyHeader
                    // BDD node 45:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760160)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                    swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                    swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                    swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                    swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                    swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                    // EP node  4931:Forward
                    // BDD node 46:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                  } else {
                    // EP node  3056:Else
                    // BDD node 40:if ((Eq (w8 1) (Read w8 (w32 768) packet_chunks))
                    // EP node  3511:Ignore
                    // BDD node 47:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 0) vector_data_384)])
                    // EP node  3912:ModifyHeader
                    // BDD node 48:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (Concat w72 (Read w8 (w32 3) vector_data_384) (Concat w64 (Read w8 (w32 2) vector_data_384) (Concat w56 (Read w8 (w32 1) vector_data_384) (Concat w48 (Read w8 (w32 0) vector_data_384) (ReadLSB w40 (w32 768) packet_chunks))))))))])
                    hdr.hdr3.data2 = vector_reg_value0;
                    hdr.hdr3.data3 = 8w0x01;
                    // EP node  4207:ModifyHeader
                    // BDD node 49:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760672)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
                    swap(hdr.hdr2.data0[15:8], hdr.hdr2.data1[15:8]);
                    swap(hdr.hdr2.data0[7:0], hdr.hdr2.data1[7:0]);
                    // EP node  4514:ModifyHeader
                    // BDD node 50:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760416)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
                    swap(hdr.hdr1.data2[31:24], hdr.hdr1.data3[31:24]);
                    swap(hdr.hdr1.data2[23:16], hdr.hdr1.data3[23:16]);
                    swap(hdr.hdr1.data2[15:8], hdr.hdr1.data3[15:8]);
                    swap(hdr.hdr1.data2[7:0], hdr.hdr1.data3[7:0]);
                    // EP node  4833:ModifyHeader
                    // BDD node 51:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760160)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
                    swap(hdr.hdr0.data0[47:40], hdr.hdr0.data1[47:40]);
                    swap(hdr.hdr0.data0[39:32], hdr.hdr0.data1[39:32]);
                    swap(hdr.hdr0.data0[31:24], hdr.hdr0.data1[31:24]);
                    swap(hdr.hdr0.data0[23:16], hdr.hdr0.data1[23:16]);
                    swap(hdr.hdr0.data0[15:8], hdr.hdr0.data1[15:8]);
                    swap(hdr.hdr0.data0[7:0], hdr.hdr0.data1[7:0]);
                    // EP node  5031:Forward
                    // BDD node 52:FORWARD
                    nf_dev[15:0] = meta.dev[15:0];
                  }
                }
              } else {
                // EP node  271:Else
                // BDD node 12:if ((Eq false (Eq (w16 0) (ReadLSB w16 (w32 0) DEVICE)))
                // EP node  3629:Forward
                // BDD node 57:FORWARD
                nf_dev[15:0] = bswap16(hdr.hdr3.data4);
              }
            }
            // EP node  133:Else
            // BDD node 10:if ((And (Or (Eq (w16 40450) (ReadLSB w16 (w32 512) packet_chunks)) (Eq (w16 40450) (ReadLSB w16 (w32 514) packet_chunks))) (Ule (w64 12) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967254) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
            // EP node  3163:ParserReject
            // BDD node 61:DROP
          }
          // EP node  63:Else
          // BDD node 8:if ((And (Eq (w8 17) (Read w8 (w32 265) packet_chunks)) (Ule (w64 8) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  2759:ParserReject
          // BDD node 64:DROP
        }
        // EP node  19:Else
        // BDD node 6:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  2183:ParserReject
        // BDD node 66:DROP
      }

    }

    forwarding_tbl.apply();
    ig_tm_md.bypass_egress = 1;
  }
}

control IngressDeparser(
  packet_out pkt,
  inout synapse_ingress_headers_t hdr,
  in    synapse_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
  Digest<hh_table_1073923800_digest_hdr>() hh_table_1073923800_digest;

  apply {
    if (ig_dprsr_md.digest_type == 1) {
      hh_table_1073923800_digest.pack({
        hdr.hdr3.data1,
      });
    }

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
  out synapse_egress_headers_t hdr,
  out synapse_egress_metadata_t eg_md,
  out egress_intrinsic_metadata_t eg_intr_md
) {
  TofinoEgressParser() tofino_parser;

  /* This is a mandatory state, required by Tofino Architecture */
  state start {
    tofino_parser.apply(pkt, eg_intr_md);
    transition accept;
  }
}

control Egress(
  inout synapse_egress_headers_t hdr,
  inout synapse_egress_metadata_t eg_md,
  in    egress_intrinsic_metadata_t eg_intr_md,
  in    egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
  inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
  inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md
) {
  apply {}
}

control EgressDeparser(
  packet_out pkt,
  inout synapse_egress_headers_t hdr,
  in    synapse_egress_metadata_t eg_md,
  in    egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md
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
