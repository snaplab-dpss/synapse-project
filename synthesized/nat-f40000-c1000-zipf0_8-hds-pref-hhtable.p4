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
  bit<32> vector_table_1074086216_139_get_value_param0;
  bit<32> dev;
  bit<64> time;
  @padding bit<31> pad_hit0;
  bool hit0;

}

header recirc_h {
  bit<16> code_path;
  bit<16> ingress_port;
  bit<32> dev;
  bit<32> vector_table_1074086216_139_get_value_param0;
  bit<32> hh_table_1074053808_table_167_get_value_param0;
  @padding bit<7> pad_hit0;
  bool hit0;

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
header hh_table_1074053808_digest_hdr {
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
      default: parser_205;
    }
  }
  state parser_136 {
    pkt.extract(hdr.hdr1);
    transition parser_137;
  }
  state parser_205 {
    transition reject;
  }
  state parser_137 {
    transition parser_137_0;
  }
  state parser_137_0 {
    transition select (hdr.hdr1.data1) {
      8w0x06: parser_138;
      8w0x11: parser_138;
      default: parser_203;
    }
  }
  state parser_138 {
    pkt.extract(hdr.hdr2);
    transition parser_166;
  }
  state parser_203 {
    transition reject;
  }
  state parser_166 {
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

  bit<32> vector_table_1074086216_139_get_value_param0 = 32w0;
  action vector_table_1074086216_139_get_value(bit<32> _vector_table_1074086216_139_get_value_param0) {
    vector_table_1074086216_139_get_value_param0 = _vector_table_1074086216_139_get_value_param0;
  }

  bit<32> vector_table_1074086216_139_key0 = 32w0;
  table vector_table_1074086216_139 {
    key = {
      vector_table_1074086216_139_key0: exact;
    }
    actions = {
      vector_table_1074086216_139_get_value;
    }
    size = 36;
  }

  bit<32> hh_table_1074053808_table_167_get_value_param0 = 32w0;
  action hh_table_1074053808_table_167_get_value(bit<32> _hh_table_1074053808_table_167_get_value_param0) {
    hh_table_1074053808_table_167_get_value_param0 = _hh_table_1074053808_table_167_get_value_param0;
  }

  bit<16> hh_table_1074053808_table_167_key0 = 16w0;
  bit<16> hh_table_1074053808_table_167_key1 = 16w0;
  bit<32> hh_table_1074053808_table_167_key2 = 32w0;
  bit<32> hh_table_1074053808_table_167_key3 = 32w0;
  bit<8> hh_table_1074053808_table_167_key4 = 8w0;
  table hh_table_1074053808_table_167 {
    key = {
      hh_table_1074053808_table_167_key0: exact;
      hh_table_1074053808_table_167_key1: exact;
      hh_table_1074053808_table_167_key2: exact;
      hh_table_1074053808_table_167_key3: exact;
      hh_table_1074053808_table_167_key4: exact;
    }
    actions = {
      hh_table_1074053808_table_167_get_value;
    }
    size = 72818;
    idle_timeout = true;
  }

  Register<bit<32>,_>(65536, 0) hh_table_1074053808_cached_counters;
  RegisterAction<bit<32>, bit<32>, void>(hh_table_1074053808_cached_counters) hh_table_1074053808_cached_counters_inc_492 = {
    void apply(inout bit<32> value) {
      value = value + 1;
    }
  };

  Register<bit<8>,_>(1, 0) hh_table_1074053808_packet_sampler;
  RegisterAction<bit<8>, bit<1>, bit<8>>(hh_table_1074053808_packet_sampler) hh_table_1074053808_packet_sampler_sample_every_fourth_492 = {
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

  Hash<bit<9>>(HashAlgorithm_t.CRC32) hh_table_1074053808_hash_calc_0;
  Hash<bit<9>>(HashAlgorithm_t.CRC32) hh_table_1074053808_hash_calc_1;
  Hash<bit<9>>(HashAlgorithm_t.CRC32) hh_table_1074053808_hash_calc_2;
  Hash<bit<9>>(HashAlgorithm_t.CRC32) hh_table_1074053808_hash_calc_3;

  Register<bit<32>,_>(512, 0) hh_table_1074053808_cms_row_0;
  RegisterAction<bit<32>, bit<9>, bit<32>>(hh_table_1074053808_cms_row_0) hh_table_1074053808_cms_row_0_inc_and_read_492 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(512, 0) hh_table_1074053808_cms_row_1;
  RegisterAction<bit<32>, bit<9>, bit<32>>(hh_table_1074053808_cms_row_1) hh_table_1074053808_cms_row_1_inc_and_read_492 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(512, 0) hh_table_1074053808_cms_row_2;
  RegisterAction<bit<32>, bit<9>, bit<32>>(hh_table_1074053808_cms_row_2) hh_table_1074053808_cms_row_2_inc_and_read_492 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(512, 0) hh_table_1074053808_cms_row_3;
  RegisterAction<bit<32>, bit<9>, bit<32>>(hh_table_1074053808_cms_row_3) hh_table_1074053808_cms_row_3_inc_and_read_492 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      value = value + 1;
      out_value = value;
    }
  };

  Register<bit<32>,_>(1, 0) hh_table_1074053808_threshold;
  bit<32> hh_table_1074053808_threshold_diff_492_cmp;
  RegisterAction<bit<32>, bit<1>, bit<32>>(hh_table_1074053808_threshold) hh_table_1074053808_threshold_diff_492 = {
    void apply(inout bit<32> value, out bit<32> out_value) {
      out_value = hh_table_1074053808_threshold_diff_492_cmp - value;
    }
  };

  bit<9> hh_table_1074053808_hash_calc_0_value;
  action hh_table_1074053808_hash_calc_0_calc() {
    hh_table_1074053808_hash_calc_0_value = hh_table_1074053808_hash_calc_0.get({
      hh_table_1074053808_table_167_key0,
      hh_table_1074053808_table_167_key1,
      hh_table_1074053808_table_167_key2,
      hh_table_1074053808_table_167_key3,
      hh_table_1074053808_table_167_key4,
      32w0xfbc31fc7
    });
  }
  bit<9> hh_table_1074053808_hash_calc_1_value;
  action hh_table_1074053808_hash_calc_1_calc() {
    hh_table_1074053808_hash_calc_1_value = hh_table_1074053808_hash_calc_1.get({
      hh_table_1074053808_table_167_key0,
      hh_table_1074053808_table_167_key1,
      hh_table_1074053808_table_167_key2,
      hh_table_1074053808_table_167_key3,
      hh_table_1074053808_table_167_key4,
      32w0x2681580b
    });
  }
  bit<9> hh_table_1074053808_hash_calc_2_value;
  action hh_table_1074053808_hash_calc_2_calc() {
    hh_table_1074053808_hash_calc_2_value = hh_table_1074053808_hash_calc_2.get({
      hh_table_1074053808_table_167_key0,
      hh_table_1074053808_table_167_key1,
      hh_table_1074053808_table_167_key2,
      hh_table_1074053808_table_167_key3,
      hh_table_1074053808_table_167_key4,
      32w0x486d7e2f
    });
  }
  bit<9> hh_table_1074053808_hash_calc_3_value;
  action hh_table_1074053808_hash_calc_3_calc() {
    hh_table_1074053808_hash_calc_3_value = hh_table_1074053808_hash_calc_3.get({
      hh_table_1074053808_table_167_key0,
      hh_table_1074053808_table_167_key1,
      hh_table_1074053808_table_167_key2,
      hh_table_1074053808_table_167_key3,
      hh_table_1074053808_table_167_key4,
      32w0x1f3a2b4d
    });
  }
  bit<32> hh_table_1074053808_cms_row_0_value;
  action hh_table_1074053808_cms_row_0_inc_and_read_492_execute() {
    hh_table_1074053808_cms_row_0_value = hh_table_1074053808_cms_row_0_inc_and_read_492.execute(hh_table_1074053808_hash_calc_0_value);
  }
  bit<32> hh_table_1074053808_cms_row_1_value;
  action hh_table_1074053808_cms_row_1_inc_and_read_492_execute() {
    hh_table_1074053808_cms_row_1_value = hh_table_1074053808_cms_row_1_inc_and_read_492.execute(hh_table_1074053808_hash_calc_1_value);
  }
  bit<32> hh_table_1074053808_cms_row_2_value;
  action hh_table_1074053808_cms_row_2_inc_and_read_492_execute() {
    hh_table_1074053808_cms_row_2_value = hh_table_1074053808_cms_row_2_inc_and_read_492.execute(hh_table_1074053808_hash_calc_2_value);
  }
  bit<32> hh_table_1074053808_cms_row_3_value;
  action hh_table_1074053808_cms_row_3_inc_and_read_492_execute() {
    hh_table_1074053808_cms_row_3_value = hh_table_1074053808_cms_row_3_inc_and_read_492.execute(hh_table_1074053808_hash_calc_3_value);
  }
  bit<16> vector_table_1074103432_191_get_value_param0 = 16w0;
  action vector_table_1074103432_191_get_value(bit<16> _vector_table_1074103432_191_get_value_param0) {
    vector_table_1074103432_191_get_value_param0 = _vector_table_1074103432_191_get_value_param0;
  }

  bit<32> vector_table_1074103432_191_key0 = 32w0;
  table vector_table_1074103432_191 {
    key = {
      vector_table_1074103432_191_key0: exact;
    }
    actions = {
      vector_table_1074103432_191_get_value;
    }
    size = 36;
  }


  apply {
    ingress_port_to_nf_dev.apply();

    if (hdr.cpu.isValid() && hdr.cpu.trigger_dataplane_execution == 0) {
      nf_dev[15:0] = hdr.cpu.egress_dev;
    } else if (hdr.recirc.isValid() && !hdr.cuckoo.isValid()) {
      if (hdr.recirc.code_path == 0) {
        // EP node  908:Ignore
        // BDD node 140:vector_return(vector:(w64 1074086216), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074100112)[(ReadLSB w32 (w32 0) vector_data_r1)])
        // EP node  1010:Recirculate
        // BDD node 142:dchain_is_index_allocated(chain:(w64 1074085792), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)))
        fwd_op = fwd_op_t.RECIRCULATE;
        build_recirc_hdr(1);
        if (hdr.recirc.code_path == 1) {
          // EP node  1041:SendToController
          // BDD node 142:dchain_is_index_allocated(chain:(w64 1074085792), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)))
          fwd_op = fwd_op_t.FORWARD_TO_CPU;
          build_cpu_hdr(1041);
          hdr.cpu.vector_table_1074086216_139_get_value_param0 = hdr.recirc.vector_table_1074086216_139_get_value_param0;
          hdr.cpu.dev = meta.dev;
          hdr.cpu.time[47:16] = meta.time;
        }
      }
      if (hdr.recirc.code_path == 2) {
        // EP node  3268:SendToController
        // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085792), index_out:(w64 1074228992)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        fwd_op = fwd_op_t.FORWARD_TO_CPU;
        build_cpu_hdr(3268);
        hdr.cpu.vector_table_1074086216_139_get_value_param0 = hdr.recirc.vector_table_1074086216_139_get_value_param0;
        hdr.cpu.hit0 = hdr.recirc.hit0;
        hdr.cpu.dev = meta.dev;
        hdr.cpu.time[47:16] = meta.time;
      }

    } else {
      // EP node  1:Ignore
      // BDD node 133:expire_items_single_map(chain:(w64 1074085792), vector:(w64 1074067632), time:(Add w64 (w64 18446744072709551616) (ReadLSB w64 (w32 0) next_time)), map:(w64 1074053808))
      // EP node  6:ParserExtraction
      // BDD node 134:packet_borrow_next_chunk(p:(w64 1074211160), length:(w32 14), chunk:(w64 1074221912)[ -> (w64 1073763984)])
      if(hdr.hdr0.isValid()) {
        // EP node  17:ParserCondition
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  18:Then
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  33:ParserExtraction
        // BDD node 136:packet_borrow_next_chunk(p:(w64 1074211160), length:(w32 20), chunk:(w64 1074222648)[ -> (w64 1073764240)])
        if(hdr.hdr1.isValid()) {
          // EP node  61:ParserCondition
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  62:Then
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  95:VectorTableLookup
          // BDD node 139:vector_borrow(vector:(w64 1074086216), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074223584)[ -> (w64 1074100112)])
          vector_table_1074086216_139_key0 = meta.dev;
          vector_table_1074086216_139.apply();
          // EP node  202:ParserExtraction
          // BDD node 138:packet_borrow_next_chunk(p:(w64 1074211160), length:(w32 4), chunk:(w64 1074223304)[ -> (w64 1073764496)])
          if(hdr.hdr2.isValid()) {
            // EP node  275:If
            // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
            if ((32w0x00000000) == (vector_table_1074086216_139_get_value_param0)) {
              // EP node  276:Then
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  787:Recirculate
              // BDD node 142:dchain_is_index_allocated(chain:(w64 1074085792), index:(ZExt w32 (ReadLSB w16 (w32 514) packet_chunks)))
              fwd_op = fwd_op_t.RECIRCULATE;
              build_recirc_hdr(0);
              hdr.recirc.vector_table_1074086216_139_get_value_param0 = vector_table_1074086216_139_get_value_param0;
            } else {
              // EP node  277:Else
              // BDD node 141:if ((Eq (w32 0) (ReadLSB w32 (w32 0) vector_data_r1))
              // EP node  350:Ignore
              // BDD node 273:vector_return(vector:(w64 1074086216), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074100112)[(ReadLSB w32 (w32 0) vector_data_r1)])
              // EP node  492:HHTableRead
              // BDD node 167:map_get(map:(w64 1074053808), key:(w64 1074221138)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value_out:(w64 1074228560)[(w32 2880154539) -> (ReadLSB w32 (w32 0) allocated_index)])
              hh_table_1074053808_table_167_key0 = hdr.hdr2.data0;
              hh_table_1074053808_table_167_key1 = hdr.hdr2.data1;
              hh_table_1074053808_table_167_key2 = hdr.hdr1.data3;
              hh_table_1074053808_table_167_key3 = hdr.hdr1.data4;
              hh_table_1074053808_table_167_key4 = hdr.hdr1.data1;
              bool hit0 = hh_table_1074053808_table_167.apply().hit;
              if (hit0) {
                hh_table_1074053808_cached_counters_inc_492.execute(hh_table_1074053808_table_167_get_value_param0);
              } else {
                bit<8> hh_table_1074053808_packet_sampler_sample_every_fourth_492_out_value = hh_table_1074053808_packet_sampler_sample_every_fourth_492.execute(0);
                if (hh_table_1074053808_packet_sampler_sample_every_fourth_492_out_value == 0) {
                  hh_table_1074053808_hash_calc_0_calc();
                  hh_table_1074053808_hash_calc_1_calc();
                  hh_table_1074053808_hash_calc_2_calc();
                  hh_table_1074053808_hash_calc_3_calc();
                  hh_table_1074053808_cms_row_0_inc_and_read_492_execute();
                  hh_table_1074053808_cms_row_1_inc_and_read_492_execute();
                  hh_table_1074053808_cms_row_2_inc_and_read_492_execute();
                  hh_table_1074053808_cms_row_3_inc_and_read_492_execute();
                  bit<32> hh_table_1074053808_cms_min = hh_table_1074053808_cms_row_0_value;
                  hh_table_1074053808_cms_min = min(hh_table_1074053808_cms_min, hh_table_1074053808_cms_row_1_value);
                  hh_table_1074053808_cms_min = min(hh_table_1074053808_cms_min, hh_table_1074053808_cms_row_2_value);
                  hh_table_1074053808_cms_min = min(hh_table_1074053808_cms_min, hh_table_1074053808_cms_row_3_value);
                  hh_table_1074053808_threshold_diff_492_cmp = hh_table_1074053808_cms_min;
                  bit<32> hh_table_1074053808_threshold_diff = hh_table_1074053808_threshold_diff_492.execute(0);
                  if (hh_table_1074053808_threshold_diff_492_cmp[31:31] == 0) {
                    ig_dprsr_md.digest_type = 2;
                  }
                }
              }
              // EP node  674:If
              // BDD node 168:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
              if (!hit0) {
                // EP node  675:Then
                // BDD node 168:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  3209:Recirculate
                // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085792), index_out:(w64 1074228992)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
                fwd_op = fwd_op_t.RECIRCULATE;
                build_recirc_hdr(2);
                hdr.recirc.vector_table_1074086216_139_get_value_param0 = vector_table_1074086216_139_get_value_param0;
                hdr.recirc.hh_table_1074053808_table_167_get_value_param0 = hh_table_1074053808_table_167_get_value_param0;
                hdr.recirc.hit0 = hit0;
              } else {
                // EP node  676:Else
                // BDD node 168:if ((Eq (w32 0) (ReadLSB w32 (w32 0) map_has_this_key))
                // EP node  1452:Ignore
                // BDD node 189:dchain_rejuvenate_index(chain:(w64 1074085792), index:(ReadLSB w32 (w32 0) allocated_index), time:(ReadLSB w64 (w32 0) next_time))
                // EP node  1595:VectorTableLookup
                // BDD node 191:vector_borrow(vector:(w64 1074103432), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074233280)[ -> (w64 1074117328)])
                vector_table_1074103432_191_key0 = meta.dev;
                vector_table_1074103432_191.apply();
                // EP node  1943:Ignore
                // BDD node 190:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073764240), l4_header:(w64 1073764496), packet:(w64 1074155944))
                // EP node  2130:Ignore
                // BDD node 192:vector_return(vector:(w64 1074103432), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074117328)[(ReadLSB w16 (w32 0) vector_data_r10)])
                // EP node  2286:If
                // BDD node 196:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r10)))
                if ((meta.dev[15:0]) != (vector_table_1074103432_191_get_value_param0)) {
                  // EP node  2287:Then
                  // BDD node 196:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r10)))
                  // EP node  2535:ModifyHeader
                  // BDD node 193:packet_return_chunk(p:(w64 1074211160), the_chunk:(w64 1073764496)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) allocated_index)))])
                  hdr.hdr2.data0[15:8] = hh_table_1074053808_table_167_get_value_param0[7:0];
                  hdr.hdr2.data0[7:0] = hh_table_1074053808_table_167_get_value_param0[15:8];
                  // EP node  2673:If
                  // BDD node 197:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r10)))
                  if ((16w0xffff) != (vector_table_1074103432_191_get_value_param0)) {
                    // EP node  2674:Then
                    // BDD node 197:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r10)))
                    // EP node  2865:ModifyHeader
                    // BDD node 194:packet_return_chunk(p:(w64 1074211160), the_chunk:(w64 1073764240)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                    hdr.hdr1.data3[31:24] = 8w0x01;
                    hdr.hdr1.data3[23:16] = 8w0x02;
                    hdr.hdr1.data3[15:8] = 8w0x03;
                    hdr.hdr1.data3[7:0] = 8w0x04;
                    // EP node  3106:Forward
                    // BDD node 198:FORWARD
                    nf_dev[15:0] = vector_table_1074103432_191_get_value_param0;
                  } else {
                    // EP node  2675:Else
                    // BDD node 197:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r10)))
                    // EP node  4078:ModifyHeader
                    // BDD node 280:packet_return_chunk(p:(w64 1074211160), the_chunk:(w64 1073764240)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                    hdr.hdr1.data3[31:24] = 8w0x01;
                    hdr.hdr1.data3[23:16] = 8w0x02;
                    hdr.hdr1.data3[15:8] = 8w0x03;
                    hdr.hdr1.data3[7:0] = 8w0x04;
                    // EP node  5282:Drop
                    // BDD node 199:DROP
                    fwd_op = fwd_op_t.DROP;
                  }
                } else {
                  // EP node  2288:Else
                  // BDD node 196:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_r10)))
                  // EP node  3873:ModifyHeader
                  // BDD node 277:packet_return_chunk(p:(w64 1074211160), the_chunk:(w64 1073764496)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) allocated_index)))])
                  hdr.hdr2.data0[15:8] = hh_table_1074053808_table_167_get_value_param0[7:0];
                  hdr.hdr2.data0[7:0] = hh_table_1074053808_table_167_get_value_param0[15:8];
                  // EP node  4599:ModifyHeader
                  // BDD node 278:packet_return_chunk(p:(w64 1074211160), the_chunk:(w64 1073764240)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
                  hdr.hdr1.data3[31:24] = 8w0x01;
                  hdr.hdr1.data3[23:16] = 8w0x02;
                  hdr.hdr1.data3[15:8] = 8w0x03;
                  hdr.hdr1.data3[7:0] = 8w0x04;
                  // EP node  5414:Drop
                  // BDD node 200:DROP
                  fwd_op = fwd_op_t.DROP;
                }
              }
            }
          }
          // EP node  63:Else
          // BDD node 137:if ((And (Or (Eq (w8 6) (Read w8 (w32 265) packet_chunks)) (Eq (w8 17) (Read w8 (w32 265) packet_chunks))) (Ule (w64 4) (ZExt w64 (Add w32 (w32 4294967262) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))))))
          // EP node  4940:ParserReject
          // BDD node 203:DROP
        }
        // EP node  19:Else
        // BDD node 135:if ((And (Eq (w16 8) (ReadLSB w16 (w32 12) packet_chunks)) (Ule (w64 20) (ZExt w64 (Extract w16 0 (Add w32 (w32 4294967282) (ZExt w32 (ReadLSB w16 (w32 0) pkt_len)))))))
        // EP node  4202:ParserReject
        // BDD node 205:DROP
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
  Digest<hh_table_1074053808_digest_hdr>() hh_table_1074053808_digest;

  apply {
    if (ig_dprsr_md.digest_type == 2) {
      hh_table_1074053808_digest.pack({
        hdr.hdr2.data0,
        hdr.hdr2.data1,
        hdr.hdr1.data3,
        hdr.hdr1.data4,
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
