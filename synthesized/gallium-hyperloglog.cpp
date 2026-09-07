#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  libnf::Vector *cpu_vector_1073917816 = nullptr;
  libnf::Vector *cpu_vector_1073935032 = nullptr;
  libnf::Vector *cpu_vector_1073952248 = nullptr;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl()
    {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());
  
  state->ingress_port_to_nf_dev.add_recirc_entry(6);
  state->ingress_port_to_nf_dev.add_recirc_entry(128);
  state->ingress_port_to_nf_dev.add_recirc_entry(256);
  state->ingress_port_to_nf_dev.add_recirc_entry(384);

  state->forwarding_tbl.add_fwd_to_cpu_entry();
  state->forwarding_tbl.add_recirc_entry(6);
  state->forwarding_tbl.add_recirc_entry(128);
  state->forwarding_tbl.add_recirc_entry(256);
  state->forwarding_tbl.add_recirc_entry(384);
  state->forwarding_tbl.add_drop_entry();

  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(3), 2);
  state->forwarding_tbl.add_fwd_nf_dev_entry(2, asic_get_dev_port(3));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(4), 3);
  state->forwarding_tbl.add_fwd_nf_dev_entry(3, asic_get_dev_port(4));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(5), 4);
  state->forwarding_tbl.add_fwd_nf_dev_entry(4, asic_get_dev_port(5));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(6), 5);
  state->forwarding_tbl.add_fwd_nf_dev_entry(5, asic_get_dev_port(6));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(7), 6);
  state->forwarding_tbl.add_fwd_nf_dev_entry(6, asic_get_dev_port(7));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(8), 7);
  state->forwarding_tbl.add_fwd_nf_dev_entry(7, asic_get_dev_port(8));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(9), 8);
  state->forwarding_tbl.add_fwd_nf_dev_entry(8, asic_get_dev_port(9));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(10), 9);
  state->forwarding_tbl.add_fwd_nf_dev_entry(9, asic_get_dev_port(10));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(11), 10);
  state->forwarding_tbl.add_fwd_nf_dev_entry(10, asic_get_dev_port(11));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(12), 11);
  state->forwarding_tbl.add_fwd_nf_dev_entry(11, asic_get_dev_port(12));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(13), 12);
  state->forwarding_tbl.add_fwd_nf_dev_entry(12, asic_get_dev_port(13));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(14), 13);
  state->forwarding_tbl.add_fwd_nf_dev_entry(13, asic_get_dev_port(14));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(15), 14);
  state->forwarding_tbl.add_fwd_nf_dev_entry(14, asic_get_dev_port(15));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(16), 15);
  state->forwarding_tbl.add_fwd_nf_dev_entry(15, asic_get_dev_port(16));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(17), 16);
  state->forwarding_tbl.add_fwd_nf_dev_entry(16, asic_get_dev_port(17));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(18), 17);
  state->forwarding_tbl.add_fwd_nf_dev_entry(17, asic_get_dev_port(18));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(19), 18);
  state->forwarding_tbl.add_fwd_nf_dev_entry(18, asic_get_dev_port(19));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(20), 19);
  state->forwarding_tbl.add_fwd_nf_dev_entry(19, asic_get_dev_port(20));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(21), 20);
  state->forwarding_tbl.add_fwd_nf_dev_entry(20, asic_get_dev_port(21));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(22), 21);
  state->forwarding_tbl.add_fwd_nf_dev_entry(21, asic_get_dev_port(22));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(23), 22);
  state->forwarding_tbl.add_fwd_nf_dev_entry(22, asic_get_dev_port(23));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(24), 23);
  state->forwarding_tbl.add_fwd_nf_dev_entry(23, asic_get_dev_port(24));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(25), 24);
  state->forwarding_tbl.add_fwd_nf_dev_entry(24, asic_get_dev_port(25));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(26), 25);
  state->forwarding_tbl.add_fwd_nf_dev_entry(25, asic_get_dev_port(26));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(27), 26);
  state->forwarding_tbl.add_fwd_nf_dev_entry(26, asic_get_dev_port(27));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(28), 27);
  state->forwarding_tbl.add_fwd_nf_dev_entry(27, asic_get_dev_port(28));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(29), 28);
  state->forwarding_tbl.add_fwd_nf_dev_entry(28, asic_get_dev_port(29));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(30), 29);
  state->forwarding_tbl.add_fwd_nf_dev_entry(29, asic_get_dev_port(30));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(31), 30);
  state->forwarding_tbl.add_fwd_nf_dev_entry(30, asic_get_dev_port(31));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(32), 31);
  state->forwarding_tbl.add_fwd_nf_dev_entry(31, asic_get_dev_port(32));
  // BDD node 0:vector_allocate(elem_size:(w32 4), capacity:(w32 64), vector_out:(w64 1073917528)[(w64 0) -> (w64 1073917816)])
  // Module VectorAllocate
  libnf::vector_allocate(4, 64, &state->cpu_vector_1073917816);
  // BDD node 1:vector_allocate(elem_size:(w32 4), capacity:(w32 1), vector_out:(w64 1073917536)[(w64 0) -> (w64 1073935032)])
  // Module VectorAllocate
  libnf::vector_allocate(4, 1, &state->cpu_vector_1073935032);
  // BDD node 2:vector_allocate(elem_size:(w32 4), capacity:(w32 1), vector_out:(w64 1073917544)[(w64 0) -> (w64 1073952248)])
  // Module VectorAllocate
  libnf::vector_allocate(4, 1, &state->cpu_vector_1073952248);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 first_set_bit__7;
  u32 hash__6;
  u32 DEVICE;

} __attribute__((packed));

nf_process_result_t sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  nf_process_result_t result;
  result.forward = true;
  bool trigger_update_ipv4_tcpudp_checksums = false;
  void* l3_hdr = nullptr;
  void* l4_hdr = nullptr;

  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);
  LOG_DEBUG("[t=%lu] New packet (size=%u, code_path=%d)\n", now, size, bswap16(cpu_hdr->code_path));

  cpu_hdr->egress_dev = 0;
  cpu_hdr->trigger_dataplane_execution = 0;



  if (bswap16(cpu_hdr->code_path) == 125) {
    // EP node  123
    // BDD node 8:vector_borrow(vector:(w64 1073917816), index:(LShr w32 (ReadLSB w32 (w32 0) hash__6) (w32 26)), val_out:(w64 1074037448)[ -> (w64 1073931712)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  124
    // BDD node 8:vector_borrow(vector:(w64 1073917816), index:(LShr w32 (ReadLSB w32 (w32 0) hash__6) (w32 26)), val_out:(w64 1074037448)[ -> (w64 1073931712)])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  181
    // BDD node 8:vector_borrow(vector:(w64 1073917816), index:(LShr w32 (ReadLSB w32 (w32 0) hash__6) (w32 26)), val_out:(w64 1074037448)[ -> (w64 1073931712)])
    u8 *vector_cell_0;
    libnf::vector_borrow(state->cpu_vector_1073917816, (bswap32(cpu_hdr_extra->hash__6)) >> (26), (void **)&vector_cell_0);
    u32 vector_value_0 = *(u32 *)vector_cell_0;
    // EP node  193
    // BDD node 9:if ((Ule (ReadLSB w32 (w32 0) first_set_bit__7) (ReadLSB w32 (w32 0) vector_data__8))
    if ((bswap32(cpu_hdr_extra->first_set_bit__7)) <= (vector_value_0)) {
      // EP node  194
      // BDD node 9:if ((Ule (ReadLSB w32 (w32 0) first_set_bit__7) (ReadLSB w32 (w32 0) vector_data__8))
      // EP node  268
      // BDD node 10:vector_return(vector:(w64 1073917816), index:(LShr w32 (ReadLSB w32 (w32 0) hash__6) (w32 26)), value:(w64 1073931712)[(ReadLSB w32 (w32 0) vector_data__8)])
      // EP node  332
      // BDD node 11:min(a:(ReadLSB w32 (w32 0) first_set_bit__7), b:(ReadLSB w32 (w32 0) vector_data__8))
      u32 minimum_0 = libnf::min(bswap32(cpu_hdr_extra->first_set_bit__7), vector_value_0);
      // EP node  400
      // BDD node 12:power_of_two(exponent:(Sub w32 (w32 20) (ReadLSB w32 (w32 0) min__11)))
      u32 power_0 = libnf::power_of_two((20) - (minimum_0));
      // EP node  454
      // BDD node 13:power_of_two(exponent:(Sub w32 (w32 20) (ReadLSB w32 (w32 0) first_set_bit__7)))
      u32 power_1 = libnf::power_of_two((20) - (bswap32(cpu_hdr_extra->first_set_bit__7)));
      // EP node  511
      // BDD node 14:vector_borrow(vector:(w64 1073935032), index:(w32 0), val_out:(w64 1074037536)[ -> (w64 1073948928)])
      u8 *vector_cell_1;
      libnf::vector_borrow(state->cpu_vector_1073935032, 0, (void **)&vector_cell_1);
      u32 vector_value_1 = *(u32 *)vector_cell_1;
      // EP node  551
      // BDD node 82:op_sub(a:(ReadLSB w32 (w32 0) power_of_two__12), b:(ReadLSB w32 (w32 0) power_of_two__13))
      u32 unrolled_0 = (power_0) - (power_1);
      // EP node  635
      // BDD node 15:vector_return(vector:(w64 1073935032), index:(w32 0), value:(w64 1073948928)[(Add w32 (ReadLSB w32 (w32 0) vector_data__14) (ReadLSB w32 (w32 0) unrolled__0))])
      vector_cell_1[0] = (u32)(((vector_value_1) + (unrolled_0)));
      vector_cell_1[1] = (u32)(((vector_value_1) + (unrolled_0))>>8);
      vector_cell_1[2] = (u32)(((vector_value_1) + (unrolled_0))>>16);
      vector_cell_1[3] = (u32)(((vector_value_1) + (unrolled_0))>>24);
      libnf::vector_return(state->cpu_vector_1073935032, 0, vector_cell_1);
      // EP node  658
      // BDD node 83:op_add(a:(ReadLSB w32 (w32 0) vector_data__14), b:(ReadLSB w32 (w32 0) unrolled__0))
      u32 unrolled_1 = (vector_value_1) + (unrolled_0);
      // EP node  704
      // BDD node 16:divide(numerator:(w32 3046596202), denominator:(Sub w32 (w32 67108864) (ReadLSB w32 (w32 0) unrolled__1)))
      u32 quotient_0 = libnf::divide(3046596202LL, (67108864) - (unrolled_1));
      // EP node  728
      // BDD node 17:vector_borrow(vector:(w64 1073952248), index:(w32 0), val_out:(w64 1074037608)[ -> (w64 1073966144)])
      u8 *vector_cell_2;
      libnf::vector_borrow(state->cpu_vector_1073952248, 0, (void **)&vector_cell_2);
      u32 vector_value_2 = *(u32 *)vector_cell_2;
      // EP node  803
      // BDD node 18:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) min__11)))
      if ((0) != (minimum_0)) {
        // EP node  804
        // BDD node 18:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) min__11)))
        // EP node  859
        // BDD node 19:vector_return(vector:(w64 1073952248), index:(w32 0), value:(w64 1073966144)[(ReadLSB w32 (w32 0) vector_data__17)])
        // EP node  917
        // BDD node 20:if ((Ult (ReadLSB w32 (w32 0) quotient__16) (w32 160))
        if ((quotient_0) < (160)) {
          // EP node  918
          // BDD node 20:if ((Ult (ReadLSB w32 (w32 0) quotient__16) (w32 160))
          // EP node  1082
          // BDD node 21:if ((Ult (ReadLSB w32 (w32 0) vector_data__17) (w32 64))
          if ((vector_value_2) < (64)) {
            // EP node  1083
            // BDD node 21:if ((Ult (ReadLSB w32 (w32 0) vector_data__17) (w32 64))
            // EP node  1119
            // BDD node 22:ln(x:(Sub w32 (w32 64) (ReadLSB w32 (w32 0) vector_data__17)), scale:(w32 64))
            u32 logarithm_0 = libnf::ln((64) - (vector_value_2), 64);
            // EP node  1233
            // BDD node 24:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Extract w8 24 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__22))) (Concat w72 (Extract w8 16 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__22))) (Concat w64 (Extract w8 8 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__22))) (Concat w56 (Extract w8 0 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__22))) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
            hdr_0[6] = (u32)(((266) - (logarithm_0)));
            hdr_0[7] = (u32)(((266) - (logarithm_0))>>8);
            hdr_0[8] = (u32)(((266) - (logarithm_0))>>16);
            hdr_0[9] = (u32)(((266) - (logarithm_0))>>24);
            hdr_0[10] = 0;
            hdr_0[11] = 0;
            // EP node  1273
            // BDD node 25:FORWARD
            cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
          } else {
            // EP node  1084
            // BDD node 21:if ((Ult (ReadLSB w32 (w32 0) vector_data__17) (w32 64))
            // EP node  4856
            // BDD node 27:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Read w8 (w32 3) quotient__16) (Concat w72 (Read w8 (w32 2) quotient__16) (Concat w64 (Read w8 (w32 1) quotient__16) (Concat w56 (Read w8 (w32 0) quotient__16) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
            hdr_0[6] = quotient_0 & 255;
            hdr_0[7] = (quotient_0>>8) & 255;
            hdr_0[8] = (quotient_0>>16) & 255;
            hdr_0[9] = (quotient_0>>24) & 255;
            hdr_0[10] = 0;
            hdr_0[11] = 0;
            // EP node  5444
            // BDD node 28:FORWARD
            cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
          }
        } else {
          // EP node  919
          // BDD node 20:if ((Ult (ReadLSB w32 (w32 0) quotient__16) (w32 160))
          // EP node  1013
          // BDD node 30:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Read w8 (w32 3) quotient__16) (Concat w72 (Read w8 (w32 2) quotient__16) (Concat w64 (Read w8 (w32 1) quotient__16) (Concat w56 (Read w8 (w32 0) quotient__16) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
          hdr_0[6] = quotient_0 & 255;
          hdr_0[7] = (quotient_0>>8) & 255;
          hdr_0[8] = (quotient_0>>16) & 255;
          hdr_0[9] = (quotient_0>>24) & 255;
          hdr_0[10] = 0;
          hdr_0[11] = 0;
          // EP node  1047
          // BDD node 31:FORWARD
          cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
        }
      } else {
        // EP node  805
        // BDD node 18:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) min__11)))
        // EP node  4317
        // BDD node 32:vector_return(vector:(w64 1073952248), index:(w32 0), value:(w64 1073966144)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__17))])
        vector_cell_2[0] = (u32)(((1) + (vector_value_2)));
        vector_cell_2[1] = (u32)(((1) + (vector_value_2))>>8);
        vector_cell_2[2] = (u32)(((1) + (vector_value_2))>>16);
        vector_cell_2[3] = (u32)(((1) + (vector_value_2))>>24);
        libnf::vector_return(state->cpu_vector_1073952248, 0, vector_cell_2);
        // EP node  4698
        // BDD node 33:if ((Ult (ReadLSB w32 (w32 0) quotient__16) (w32 160))
        if ((quotient_0) < (160)) {
          // EP node  4699
          // BDD node 33:if ((Ult (ReadLSB w32 (w32 0) quotient__16) (w32 160))
          // EP node  5186
          // BDD node 34:if ((Ult (Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__17)) (w32 64))
          if (((1) + (vector_value_2)) < (64)) {
            // EP node  5187
            // BDD node 34:if ((Ult (Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__17)) (w32 64))
            // EP node  5712
            // BDD node 35:ln(x:(Sub w32 (w32 63) (ReadLSB w32 (w32 0) vector_data__17)), scale:(w32 64))
            u32 logarithm_1 = libnf::ln((63) - (vector_value_2), 64);
            // EP node  6359
            // BDD node 37:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Extract w8 24 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__35))) (Concat w72 (Extract w8 16 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__35))) (Concat w64 (Extract w8 8 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__35))) (Concat w56 (Extract w8 0 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__35))) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
            hdr_0[6] = (u32)(((266) - (logarithm_1)));
            hdr_0[7] = (u32)(((266) - (logarithm_1))>>8);
            hdr_0[8] = (u32)(((266) - (logarithm_1))>>16);
            hdr_0[9] = (u32)(((266) - (logarithm_1))>>24);
            hdr_0[10] = 0;
            hdr_0[11] = 0;
            // EP node  6552
            // BDD node 38:FORWARD
            cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
          } else {
            // EP node  5188
            // BDD node 34:if ((Ult (Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__17)) (w32 64))
            // EP node  6170
            // BDD node 40:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Read w8 (w32 3) quotient__16) (Concat w72 (Read w8 (w32 2) quotient__16) (Concat w64 (Read w8 (w32 1) quotient__16) (Concat w56 (Read w8 (w32 0) quotient__16) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
            hdr_0[6] = quotient_0 & 255;
            hdr_0[7] = (quotient_0>>8) & 255;
            hdr_0[8] = (quotient_0>>16) & 255;
            hdr_0[9] = (quotient_0>>24) & 255;
            hdr_0[10] = 0;
            hdr_0[11] = 0;
            // EP node  6455
            // BDD node 41:FORWARD
            cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
          }
        } else {
          // EP node  4700
          // BDD node 33:if ((Ult (ReadLSB w32 (w32 0) quotient__16) (w32 160))
          // EP node  5985
          // BDD node 43:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Read w8 (w32 3) quotient__16) (Concat w72 (Read w8 (w32 2) quotient__16) (Concat w64 (Read w8 (w32 1) quotient__16) (Concat w56 (Read w8 (w32 0) quotient__16) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
          hdr_0[6] = quotient_0 & 255;
          hdr_0[7] = (quotient_0>>8) & 255;
          hdr_0[8] = (quotient_0>>16) & 255;
          hdr_0[9] = (quotient_0>>24) & 255;
          hdr_0[10] = 0;
          hdr_0[11] = 0;
          // EP node  6264
          // BDD node 44:FORWARD
          cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
        }
      }
    } else {
      // EP node  195
      // BDD node 9:if ((Ule (ReadLSB w32 (w32 0) first_set_bit__7) (ReadLSB w32 (w32 0) vector_data__8))
      // EP node  1474
      // BDD node 45:vector_return(vector:(w64 1073917816), index:(LShr w32 (ReadLSB w32 (w32 0) hash__6) (w32 26)), value:(w64 1073931712)[(ReadLSB w32 (w32 0) first_set_bit__7)])
      vector_cell_0[0] = bswap32(cpu_hdr_extra->first_set_bit__7) & 255;
      vector_cell_0[1] = (bswap32(cpu_hdr_extra->first_set_bit__7)>>8) & 255;
      vector_cell_0[2] = (bswap32(cpu_hdr_extra->first_set_bit__7)>>16) & 255;
      vector_cell_0[3] = (bswap32(cpu_hdr_extra->first_set_bit__7)>>24) & 255;
      libnf::vector_return(state->cpu_vector_1073917816, (bswap32(cpu_hdr_extra->hash__6)) >> (26), vector_cell_0);
      // EP node  1598
      // BDD node 46:min(a:(ReadLSB w32 (w32 0) first_set_bit__7), b:(ReadLSB w32 (w32 0) vector_data__8))
      u32 minimum_1 = libnf::min(bswap32(cpu_hdr_extra->first_set_bit__7), vector_value_0);
      // EP node  1766
      // BDD node 47:power_of_two(exponent:(Sub w32 (w32 20) (ReadLSB w32 (w32 0) min__46)))
      u32 power_2 = libnf::power_of_two((20) - (minimum_1));
      // EP node  1895
      // BDD node 48:power_of_two(exponent:(Sub w32 (w32 20) (ReadLSB w32 (w32 0) first_set_bit__7)))
      u32 power_3 = libnf::power_of_two((20) - (bswap32(cpu_hdr_extra->first_set_bit__7)));
      // EP node  2027
      // BDD node 49:vector_borrow(vector:(w64 1073935032), index:(w32 0), val_out:(w64 1074037536)[ -> (w64 1073948928)])
      u8 *vector_cell_3;
      libnf::vector_borrow(state->cpu_vector_1073935032, 0, (void **)&vector_cell_3);
      u32 vector_value_3 = *(u32 *)vector_cell_3;
      // EP node  2117
      // BDD node 84:op_sub(a:(ReadLSB w32 (w32 0) power_of_two__47), b:(ReadLSB w32 (w32 0) power_of_two__48))
      u32 unrolled_2 = (power_2) - (power_3);
      // EP node  2301
      // BDD node 50:vector_return(vector:(w64 1073935032), index:(w32 0), value:(w64 1073948928)[(Add w32 (ReadLSB w32 (w32 0) vector_data__49) (ReadLSB w32 (w32 0) unrolled__2))])
      vector_cell_3[0] = (u32)(((vector_value_3) + (unrolled_2)));
      vector_cell_3[1] = (u32)(((vector_value_3) + (unrolled_2))>>8);
      vector_cell_3[2] = (u32)(((vector_value_3) + (unrolled_2))>>16);
      vector_cell_3[3] = (u32)(((vector_value_3) + (unrolled_2))>>24);
      libnf::vector_return(state->cpu_vector_1073935032, 0, vector_cell_3);
      // EP node  2349
      // BDD node 85:op_add(a:(ReadLSB w32 (w32 0) vector_data__49), b:(ReadLSB w32 (w32 0) unrolled__2))
      u32 unrolled_3 = (vector_value_3) + (unrolled_2);
      // EP node  2445
      // BDD node 51:divide(numerator:(w32 3046596202), denominator:(Sub w32 (w32 67108864) (ReadLSB w32 (w32 0) unrolled__3)))
      u32 quotient_1 = libnf::divide(3046596202LL, (67108864) - (unrolled_3));
      // EP node  2494
      // BDD node 52:vector_borrow(vector:(w64 1073952248), index:(w32 0), val_out:(w64 1074037608)[ -> (w64 1073966144)])
      u8 *vector_cell_4;
      libnf::vector_borrow(state->cpu_vector_1073952248, 0, (void **)&vector_cell_4);
      u32 vector_value_4 = *(u32 *)vector_cell_4;
      // EP node  2644
      // BDD node 53:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) min__46)))
      if ((0) != (minimum_1)) {
        // EP node  2645
        // BDD node 53:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) min__46)))
        // EP node  2750
        // BDD node 54:vector_return(vector:(w64 1073952248), index:(w32 0), value:(w64 1073966144)[(ReadLSB w32 (w32 0) vector_data__52)])
        // EP node  2858
        // BDD node 55:if ((Ult (ReadLSB w32 (w32 0) quotient__51) (w32 160))
        if ((quotient_1) < (160)) {
          // EP node  2859
          // BDD node 55:if ((Ult (ReadLSB w32 (w32 0) quotient__51) (w32 160))
          // EP node  3800
          // BDD node 56:if ((Ult (ReadLSB w32 (w32 0) vector_data__52) (w32 64))
          if ((vector_value_4) < (64)) {
            // EP node  3801
            // BDD node 56:if ((Ult (ReadLSB w32 (w32 0) vector_data__52) (w32 64))
            // EP node  3872
            // BDD node 57:ln(x:(Sub w32 (w32 64) (ReadLSB w32 (w32 0) vector_data__52)), scale:(w32 64))
            u32 logarithm_2 = libnf::ln((64) - (vector_value_4), 64);
            // EP node  4091
            // BDD node 59:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Extract w8 24 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__57))) (Concat w72 (Extract w8 16 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__57))) (Concat w64 (Extract w8 8 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__57))) (Concat w56 (Extract w8 0 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__57))) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
            hdr_0[6] = (u32)(((266) - (logarithm_2)));
            hdr_0[7] = (u32)(((266) - (logarithm_2))>>8);
            hdr_0[8] = (u32)(((266) - (logarithm_2))>>16);
            hdr_0[9] = (u32)(((266) - (logarithm_2))>>24);
            hdr_0[10] = 0;
            hdr_0[11] = 0;
            // EP node  4166
            // BDD node 60:FORWARD
            cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
          } else {
            // EP node  3802
            // BDD node 56:if ((Ult (ReadLSB w32 (w32 0) vector_data__52) (w32 64))
            // EP node  5102
            // BDD node 62:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Read w8 (w32 3) quotient__51) (Concat w72 (Read w8 (w32 2) quotient__51) (Concat w64 (Read w8 (w32 1) quotient__51) (Concat w56 (Read w8 (w32 0) quotient__51) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
            hdr_0[6] = quotient_1 & 255;
            hdr_0[7] = (quotient_1>>8) & 255;
            hdr_0[8] = (quotient_1>>16) & 255;
            hdr_0[9] = (quotient_1>>24) & 255;
            hdr_0[10] = 0;
            hdr_0[11] = 0;
            // EP node  5711
            // BDD node 63:FORWARD
            cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
          }
        } else {
          // EP node  2860
          // BDD node 55:if ((Ult (ReadLSB w32 (w32 0) quotient__51) (w32 160))
          // EP node  3029
          // BDD node 65:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Read w8 (w32 3) quotient__51) (Concat w72 (Read w8 (w32 2) quotient__51) (Concat w64 (Read w8 (w32 1) quotient__51) (Concat w56 (Read w8 (w32 0) quotient__51) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
          hdr_0[6] = quotient_1 & 255;
          hdr_0[7] = (quotient_1>>8) & 255;
          hdr_0[8] = (quotient_1>>16) & 255;
          hdr_0[9] = (quotient_1>>24) & 255;
          hdr_0[10] = 0;
          hdr_0[11] = 0;
          // EP node  3088
          // BDD node 66:FORWARD
          cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
        }
      } else {
        // EP node  2646
        // BDD node 53:if ((Eq false (Eq (w32 0) (ReadLSB w32 (w32 0) min__46)))
        // EP node  3207
        // BDD node 67:vector_return(vector:(w64 1073952248), index:(w32 0), value:(w64 1073966144)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__52))])
        vector_cell_4[0] = (u32)(((1) + (vector_value_4)));
        vector_cell_4[1] = (u32)(((1) + (vector_value_4))>>8);
        vector_cell_4[2] = (u32)(((1) + (vector_value_4))>>16);
        vector_cell_4[3] = (u32)(((1) + (vector_value_4))>>24);
        libnf::vector_return(state->cpu_vector_1073952248, 0, vector_cell_4);
        // EP node  3268
        // BDD node 68:if ((Ult (ReadLSB w32 (w32 0) quotient__51) (w32 160))
        if ((quotient_1) < (160)) {
          // EP node  3269
          // BDD node 68:if ((Ult (ReadLSB w32 (w32 0) quotient__51) (w32 160))
          // EP node  3394
          // BDD node 69:if ((Ult (Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__52)) (w32 64))
          if (((1) + (vector_value_4)) < (64)) {
            // EP node  3395
            // BDD node 69:if ((Ult (Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__52)) (w32 64))
            // EP node  3460
            // BDD node 70:ln(x:(Sub w32 (w32 63) (ReadLSB w32 (w32 0) vector_data__52)), scale:(w32 64))
            u32 logarithm_3 = libnf::ln((63) - (vector_value_4), 64);
            // EP node  3661
            // BDD node 72:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Extract w8 24 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__70))) (Concat w72 (Extract w8 16 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__70))) (Concat w64 (Extract w8 8 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__70))) (Concat w56 (Extract w8 0 (Sub w32 (w32 266) (ReadLSB w32 (w32 0) ln__70))) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
            hdr_0[6] = (u32)(((266) - (logarithm_3)));
            hdr_0[7] = (u32)(((266) - (logarithm_3))>>8);
            hdr_0[8] = (u32)(((266) - (logarithm_3))>>16);
            hdr_0[9] = (u32)(((266) - (logarithm_3))>>24);
            hdr_0[10] = 0;
            hdr_0[11] = 0;
            // EP node  3730
            // BDD node 73:FORWARD
            cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
          } else {
            // EP node  3396
            // BDD node 69:if ((Ult (Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__52)) (w32 64))
            // EP node  5019
            // BDD node 75:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Read w8 (w32 3) quotient__51) (Concat w72 (Read w8 (w32 2) quotient__51) (Concat w64 (Read w8 (w32 1) quotient__51) (Concat w56 (Read w8 (w32 0) quotient__51) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
            hdr_0[6] = quotient_1 & 255;
            hdr_0[7] = (quotient_1>>8) & 255;
            hdr_0[8] = (quotient_1>>16) & 255;
            hdr_0[9] = (quotient_1>>24) & 255;
            hdr_0[10] = 0;
            hdr_0[11] = 0;
            // EP node  5621
            // BDD node 76:FORWARD
            cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
          }
        } else {
          // EP node  3270
          // BDD node 68:if ((Ult (ReadLSB w32 (w32 0) quotient__51) (w32 160))
          // EP node  4937
          // BDD node 78:packet_return_chunk(p:(w64 1074027408), the_chunk:(w64 1073756096)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (w8 0) (Concat w88 (w8 0) (Concat w80 (Read w8 (w32 3) quotient__51) (Concat w72 (Read w8 (w32 2) quotient__51) (Concat w64 (Read w8 (w32 1) quotient__51) (Concat w56 (Read w8 (w32 0) quotient__51) (ReadLSB w48 (w32 0) packet_chunks)))))))))])
          hdr_0[6] = quotient_1 & 255;
          hdr_0[7] = (quotient_1>>8) & 255;
          hdr_0[8] = (quotient_1>>16) & 255;
          hdr_0[9] = (quotient_1>>24) & 255;
          hdr_0[10] = 0;
          hdr_0[11] = 0;
          // EP node  5532
          // BDD node 79:FORWARD
          cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
        }
      }
    }
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
