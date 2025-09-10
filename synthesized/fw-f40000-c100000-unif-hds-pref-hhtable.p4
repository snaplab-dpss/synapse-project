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
  bit<32> vector_table_1074077160_139_get_value_param0;
  bit<32> dev;
  bit<64> time;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> vector_table_1074077160_139_get_value_param0;

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
  bit<96> data0;
  bit<16> data1;
}
header hdr1_h {
  bit<72> data0;
  bit<8> data1;
  bit<16> data2;
  bit<32> data3;
  bit<32> data4;
}
header hdr2_h {
  bit<16> data0;
  bit<16> data1;
}
header hh_table_1074044752_digest_hdr {
  bit<16> data0;
  bit<16> data1;
  bit<32> data2;
  bit<32> data3;
  bit<8> data4;
}



struct synapse_ingress_headers_t {
  cpu_h cpu;
  recirc_h recirc;
  cuckoo_h cuckoo;
  hdr0_h hdr0;
  hdr1_h hdr1;
  hdr2_h hdr2;

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
    transition parser_135;
  }
  state parser_135 {
    transition parser_135_0;
  }
  state parser_135_0 {
    transition select (hdr.hdr0.data1) {
      16w0x0800: parser_136;
      default: parser_201;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr1);
    transition parser_137;
  }
  state parser_201 {
    transition reject;
  }
  state parser_137 {
    transition parser_137_0;
  }
  state parser_137_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_138;
      8w0x11: parser_138;
      default: parser_199;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr2);
    transition parser_147;
  }
  state parser_199 {
    transition reject;
  }
  state parser_147 {
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

  bit<32> vector_table_1074077160_139_get_value_param0 = 32w0;
  action vector_table_1074077160_139_get_value(bit<32> _vector_table_1074077160_139_get_value_param0) {
    vector_table_1074077160_139_get_value_param0 = _vector_table_1074077160_139_get_value_param0;
  }

  bit<32> vector_table_1074077160_139_key0 = 32w0;
  table vector_table_1074077160_139 {
    key = {
      vector_table_1074077160_139_key0: exact;
    }
    actions = {
      vector_table_1074077160_139_get_value;
    }
    size = 36;
  }

  bit<32> hh_table_1074044752_table_142_get_value_param0 = 32w0;
  action hh_table_1074044752_table_142_get_value(bit<32> _hh_table_1074044752_table_142_get_value_param0) {
    hh_table_1074044752_table_142_get_value_param0 = _hh_table_1074044752_table_142_get_value_param0;
  }

  bit<16> hh_table_1074044752_table_142_key0 = 16w0;
  bit<16> hh_table_1074044752_table_142_key1 = 16w0;
  bit<32> hh_table_1074044752_table_142_key2 = 32w0;
  bit<32> hh_table_1074044752_table_142_key3 = 32w0;
  bit<8> hh_table_1074044752_table_142_key4 = 8w0;
  table hh_table_1074044752_table_142 {
    key = {
      hh_table_1074044752_table_142_key0: exact;
      hh_table_1074044752_table_142_key1: exact;
      hh_table_1074044752_table_142_key2: exact;
      hh_table_1074044752_table_142_key3: exact;
      hh_table_1074044752_table_142_key4: exact;
    }
    actions = {
      hh_table_1074044752_table_142_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  Register<bit<32>,_>(65536, 0) hh_table_1074044752_cached_counters;
  RegisterAction<bit<32>, bit<32>, void>(hh_table_1074044752_cached_counters) hh_table_1074044752_cached_counters_inc_440 = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  Register<bit<8>,_>(1, 0) hh_table_1074044752_packet_sampler;
  RegisterAction<bit<8>, bit<1>, bit<8>>(hh_table_1074044752_packet_sampler) hh_table_1074044752_packet_sampler_sample_every_fourth_440 = {
    void apply(inout bit<8> value, out bit<8> out_value) {
      out_value = 0;
      if (value < 3) {
        value = value + 1;
      } else {
        value = 0;
        out_value = 1;
      }
    }
  };

  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1074044752_hash_calc_0;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1074044752_hash_calc_1;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1074044752_hash_calc_2;
  Hash<bit<10>>(HashAlgorithm_t.CRC32) hh_table_1074044752_hash_calc_3;

  Register<bit<32>,_>(1024, 0) hh_table_1074044752_cms_row_0;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1074044752_cms_row_0) hh_table_1074044752_cms_row_0_inc_and_read_440 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1024, 0) hh_table_1074044752_cms_row_1;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1074044752_cms_row_1) hh_table_1074044752_cms_row_1_inc_and_read_440 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1024, 0) hh_table_1074044752_cms_row_2;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1074044752_cms_row_2) hh_table_1074044752_cms_row_2_inc_and_read_440 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1024, 0) hh_table_1074044752_cms_row_3;
  RegisterAction<bit<32>, bit<10>, bit<32>>(hh_table_1074044752_cms_row_3) hh_table_1074044752_cms_row_3_inc_and_read_440 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1, 0) hh_table_1074044752_threshold;
  bit<32> hh_table_1074044752_threshold_diff_440_cmp;
  RegisterAction<bit<32>, bit<1>, bit<32>>(hh_table_1074044752_threshold) hh_table_1074044752_threshold_diff_440 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = hh_table_1074044752_threshold_diff_440_cmp - value;
    }
  };

  bit<10> hh_table_1074044752_hash_calc_0_value;
  action hh_table_1074044752_hash_calc_0_calc() {
    hh_table_1074044752_hash_calc_0_value = hh_table_1074044752_hash_calc_0.get({
      hh_table_1074044752_table_142_key0,
      hh_table_1074044752_table_142_key1,
      hh_table_1074044752_table_142_key2,
      hh_table_1074044752_table_142_key3,
      hh_table_1074044752_table_142_key4,
      32w0xfbc31fc7
    });
  }
  bit<10> hh_table_1074044752_hash_calc_1_value;
  action hh_table_1074044752_hash_calc_1_calc() {
    hh_table_1074044752_hash_calc_1_value = hh_table_1074044752_hash_calc_1.get({
      hh_table_1074044752_table_142_key0,
      hh_table_1074044752_table_142_key1,
      hh_table_1074044752_table_142_key2,
      hh_table_1074044752_table_142_key3,
      hh_table_1074044752_table_142_key4,
      32w0x2681580b
    });
  }
  bit<10> hh_table_1074044752_hash_calc_2_value;
  action hh_table_1074044752_hash_calc_2_calc() {
    hh_table_1074044752_hash_calc_2_value = hh_table_1074044752_hash_calc_2.get({
      hh_table_1074044752_table_142_key0,
      hh_table_1074044752_table_142_key1,
      hh_table_1074044752_table_142_key2,
      hh_table_1074044752_table_142_key3,
      hh_table_1074044752_table_142_key4,
      32w0x486d7e2f
    });
  }
  bit<10> hh_table_1074044752_hash_calc_3_value;
  action hh_table_1074044752_hash_calc_3_calc() {
    hh_table_1074044752_hash_calc_3_value = hh_table_1074044752_hash_calc_3.get({
      hh_table_1074044752_table_142_key0,
      hh_table_1074044752_table_142_key1,
      hh_table_1074044752_table_142_key2,
      hh_table_1074044752_table_142_key3,
      hh_table_1074044752_table_142_key4,
      32w0x1f3a2b4d
    });
  }
  bit<32> hh_table_1074044752_cms_row_0_value;
  action hh_table_1074044752_cms_row_0_inc_and_read_440_execute() {
    hh_table_1074044752_cms_row_0_value = hh_table_1074044752_cms_row_0_inc_and_read_440.execute(hh_table_1074044752_hash_calc_0_value);
  }
  bit<32> hh_table_1074044752_cms_row_1_value;
  action hh_table_1074044752_cms_row_1_inc_and_read_440_execute() {
    hh_table_1074044752_cms_row_1_value = hh_table_1074044752_cms_row_1_inc_and_read_440.execute(hh_table_1074044752_hash_calc_1_value);
  }
  bit<32> hh_table_1074044752_cms_row_2_value;
  action hh_table_1074044752_cms_row_2_inc_and_read_440_execute() {
    hh_table_1074044752_cms_row_2_value = hh_table_1074044752_cms_row_2_inc_and_read_440.execute(hh_table_1074044752_hash_calc_2_value);
  }
  bit<32> hh_table_1074044752_cms_row_3_value;
  action hh_table_1074044752_cms_row_3_inc_and_read_440_execute() {
    hh_table_1074044752_cms_row_3_value = hh_table_1074044752_cms_row_3_inc_and_read_440.execute(hh_table_1074044752_hash_calc_3_value);
  }
  bit<16> vector_table_1074094376_149_get_value_param0 = 16w0;
  action vector_table_1074094376_149_get_value(bit<16> _vector_table_1074094376_149_get_value_param0) {
    vector_table_1074094376_149_get_value_param0 = _vector_table_1074094376_149_get_value_param0;
  }

  bit<32> vector_table_1074094376_149_key0 = 32w0;
  table vector_table_1074094376_149 {
    key = {
      vector_table_1074094376_149_key0: exact;
    }
    actions = {
      vector_table_1074094376_149_get_value;
    }
    size = 36;
  }


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  1982:Recirculate
        // BDD node 159:map_get(map:(w64 1074044752), key:(w64 1074211730)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074218072)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(1);
        if (hdr.recirc.code_path == 1) {
          // EP node  2192:SendToController
          // BDD node 159:map_get(map:(w64 1074044752), key:(w64 1074211730)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074218072)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
          fwd_op = fwd_op_t.FORWARD_TO_CPU;
          build_cpu_hdr(2192);
          hdr.cpu.vector_table_1074077160_139_get_value_param0 = hdr.recirc.vector_table_1074077160_139_get_value_param0;
          hdr.cpu.dev = meta.dev;
          hdr.cpu.time[47:16] = meta.time;
        }
      }

    } else {
      // EP node  1:Ignore
      // BDD node 133:expire_items_single_map(chain:(w64 1074076736), vector:(w64 1074058576), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074044752))
      // EP node  6:ParserExtraction
      // BDD node 134:packet_borrow_next_chunk(p:(w64 1074201752), length:(w32 14), chunk:(w64 1074212456)[ -> (w64 1073762880)])
      if(hdr.hdr0.isValid()) {
        // EP node  17:ParserCondition
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  18:Then
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  33:ParserExtraction
        // BDD node 136:packet_borrow_next_chunk(p:(w64 1074201752), length:(w32 20), chunk:(w64 1074213192)[ -> (w64 1073763136)])
        if(hdr.hdr1.isValid()) {
          // EP node  61:ParserCondition
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  62:Then
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  95:VectorTableLookup
          // BDD node 139:vector_borrow(vector:(w64 1074077160), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074214128)[ -> (w64 1074091056)])
          vector_table_1074077160_139_key0 = meta.dev;
          vector_table_1074077160_139.apply();
          // EP node  202:ParserExtraction
          // BDD node 138:packet_borrow_next_chunk(p:(w64 1074201752), length:(w32 4), chunk:(w64 1074213848)[ -> (w64 1073763392)])
          if(hdr.hdr2.isValid()) {
            // EP node  264:Ignore
            // BDD node 140:vector_return(vector:(w64 1074077160), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074091056)[(ReadLSB w32 (w32 0) vector_data_r1)])
            // EP node  306:If
            // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
            if ((32w0x00000000) == (vector_table_1074077160_139_get_value_param0)) {
              // EP node  307:Then
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  440:HHTableRead
              // BDD node 142:map_get(map:(w64 1074044752), key:(w64 1074211753)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 271) packet_chunks) (Concat w88 (Read w8 (w32 270) packet_chunks) (Concat w80 (Read w8 (w32 269) packet_chunks) (Concat w72 (Read w8 (w32 268) packet_chunks) (Concat w64 (Read w8 (w32 275) packet_chunks) (Concat w56 (Read w8 (w32 274) packet_chunks) (Concat w48 (Read w8 (w32 273) packet_chunks) (Concat w40 (Read w8 (w32 272) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 271) packet_chunks) (Concat w88 (Read w8 (w32 270) packet_chunks) (Concat w80 (Read w8 (w32 269) packet_chunks) (Concat w72 (Read w8 (w32 268) packet_chunks) (Concat w64 (Read w8 (w32 275) packet_chunks) (Concat w56 (Read w8 (w32 274) packet_chunks) (Concat w48 (Read w8 (w32 273) packet_chunks) (Concat w40 (Read w8 (w32 272) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks))))))))))))], value_out:(w64 1074214760)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              hh_table_1074044752_table_142_key0 = hdr.hdr2.data1;
              hh_table_1074044752_table_142_key1 = hdr.hdr2.data0;
              hh_table_1074044752_table_142_key2 = hdr.hdr1.data4;
              hh_table_1074044752_table_142_key3 = hdr.hdr1.data3;
              hh_table_1074044752_table_142_key4 = hdr.hdr1.data1;
              bool hit0 = hh_table_1074044752_table_142.apply().hit;
              if (hit0) {
                hh_table_1074044752_cached_counters_inc_440.execute(hh_table_1074044752_table_142_get_value_param0);
              } else {
                bit<8> hh_table_1074044752_packet_sampler_sample_every_fourth_440_out_value = hh_table_1074044752_packet_sampler_sample_every_fourth_440.execute(0);
                if (hh_table_1074044752_packet_sampler_sample_every_fourth_440_out_value == 0) {
                  hh_table_1074044752_hash_calc_0_calc();
                  hh_table_1074044752_hash_calc_1_calc();
                  hh_table_1074044752_hash_calc_2_calc();
                  hh_table_1074044752_hash_calc_3_calc();
                  hh_table_1074044752_cms_row_0_inc_and_read_440_execute();
                  hh_table_1074044752_cms_row_1_inc_and_read_440_execute();
                  hh_table_1074044752_cms_row_2_inc_and_read_440_execute();
                  hh_table_1074044752_cms_row_3_inc_and_read_440_execute();
                  bit<32> hh_table_1074044752_cms_min = hh_table_1074044752_cms_row_0_value;
                  hh_table_1074044752_cms_min = min(hh_table_1074044752_cms_min, hh_table_1074044752_cms_row_1_value);
                  hh_table_1074044752_cms_min = min(hh_table_1074044752_cms_min, hh_table_1074044752_cms_row_2_value);
                  hh_table_1074044752_cms_min = min(hh_table_1074044752_cms_min, hh_table_1074044752_cms_row_3_value);
                  hh_table_1074044752_threshold_diff_440_cmp = hh_table_1074044752_cms_min;
                  bit<32> hh_table_1074044752_threshold_diff = hh_table_1074044752_threshold_diff_440.execute(0);
                  if (hh_table_1074044752_threshold_diff_440_cmp[31:31] == 0) {
                    ig_dprsr_md.digest_type = 2;
                  }
                }
              }
              // EP node  1884:If
              // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit0) {
                // EP node  1885:Then
                // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  5143:Drop
                // BDD node 147:DROP
                fwd_op = fwd_op_t.DROP;
              } else {
                // EP node  1886:Else
                // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  3400:Ignore
                // BDD node 148:dchain_rejuvenate_index(chain:(w64 1074076736), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                // EP node  3508:VectorTableLookup
                // BDD node 149:vector_borrow(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074215976)[ -> (w64 1074108272)])
                vector_table_1074094376_149_key0 = meta.dev;
                vector_table_1074094376_149.apply();
                // EP node  3749:Ignore
                // BDD node 150:vector_return(vector:(w64 1074094376), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074108272)[(ReadLSB w16 (w32 0) vector_data_640)])
                // EP node  4410:If
                // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
                if ((meta.dev[15:0]) != (vector_table_1074094376_149_get_value_param0)) {
                  // EP node  4411:Then
                  // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
                  // EP node  4538:If
                  // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                  if ((16w0xffff) != (vector_table_1074094376_149_get_value_param0)) {
                    // EP node  4539:Then
                    // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                    // EP node  4656:Forward
                    // BDD node 156:FORWARD
                    nf_dev[15:0] = vector_table_1074094376_149_get_value_param0;
                  } else {
                    // EP node  4540:Else
                    // BDD node 155:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
                    // EP node  5566:Drop
                    // BDD node 157:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  4412:Else
                  // BDD node 154:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
                  // EP node  5480:Drop
                  // BDD node 158:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            } else {
              // EP node  308:Else
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  950:Recirculate
              // BDD node 159:map_get(map:(w64 1074044752), key:(w64 1074211730)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074218072)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              fwd_op = fwd_op_t.RECIRCULATE;
              build_recirc_hdr(0);
              hdr.recirc.vector_table_1074077160_139_get_value_param0 = vector_table_1074077160_139_get_value_param0;
            }
          }
          // EP node  63:Else
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  5880:ParserReject
          // BDD node 199:DROP
        }
        // EP node  19:Else
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  5654:ParserReject
        // BDD node 201:DROP
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
  Digest<hh_table_1074044752_digest_hdr>() hh_table_1074044752_digest;

  apply {
    if (ig_dprsr_md.digest_type == 2) {
      hh_table_1074044752_digest.pack({
        hdr.hdr2.data1,
        hdr.hdr2.data0,
        hdr.hdr1.data4,
        hdr.hdr1.data3,
        hdr.hdr1.data1,
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
