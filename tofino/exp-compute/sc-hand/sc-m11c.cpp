#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  BloomFilter bf_1073926928;
  VectorRegister vector_register_1073939504;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      bf_1073926928("bf_1073926928",{"Ingress.bf_1073926928_row_0", "Ingress.bf_1073926928_row_1", }, 0LL),
      vector_register_1073939504("vector_register_1073939504",{"Ingress.vector_register_1073939504_0",})
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

  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(1), 0);
  state->forwarding_tbl.add_fwd_nf_dev_entry(0, asic_get_dev_port(1));
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
  // BDD node 0:bf_allocate(height:(w32 2), width:(w32 1048576), key_size:(w16 12), cleanup_interval:(w64 0), bf_out:(w64 1073926656)[(w64 0) -> (w64 1073926928)])
  // Module DataplaneBloomFilterAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 4), capacity:(w32 1), vector_out:(w64 1073926664)[(w64 0) -> (w64 1073939504)])
  // Module DataplaneVectorRegisterAllocate

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 bf_query_estimate__14;
  u32 unrolled__104;
  u32 unrolled__105;
  // m11c: the data plane's state header follows the cpu header; each word named by the value it
  // holds at the hand-off (s32_0..s32_10), unread words padded.
  u32 unrolled__100;   // s32_0
  u32 rotated__85;     // s32_1
  u32 unrolled__106;   // s32_2
  u32 st_s32_3;
  u32 rotated__84;     // s32_4
  u32 unrolled__101;   // s32_5
  u32 st_s32_6;
  u32 st_s32_7;
  u32 st_s32_8;
  u32 rotated__86;     // s32_9
  u32 rotated__87;     // s32_10

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



  if (bswap16(cpu_hdr->code_path) == 0) {
    // EP node  595621
    // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  595622
    // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  595623
    // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
    u8* hdr_2 = packet_consume(pkt, 20);
    // EP node  595624
    // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
    if ((0) == (((u8)(*(u8*)(hdr_2 + 13))) & (2))) {
      // EP node  595625
      // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
      // EP node  595628
      // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
      if ((0) == (bswap32(cpu_hdr_extra->bf_query_estimate__14))) {
        // EP node  595629
        // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
        // EP node  657161
        // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
        u32 unrolled_0 = (bswap32(cpu_hdr_extra->rotated__84)) + (bswap32(cpu_hdr_extra->unrolled__106));
        // EP node  658075
        // BDD node 339:op_add(a:(ReadLSB w32 (w32 0) unrolled__100), b:(ReadLSB w32 (w32 0) unrolled__101))
        u32 unrolled_1 = (bswap32(cpu_hdr_extra->unrolled__100)) + (bswap32(cpu_hdr_extra->unrolled__101));
        // EP node  658991
        // BDD node 340:op_xor(a:(ReadLSB w32 (w32 0) rotated__85), b:(ReadLSB w32 (w32 0) unrolled__108))
        u32 unrolled_2 = (bswap32(cpu_hdr_extra->rotated__85)) ^ (unrolled_1);
        // EP node  659909
        // BDD node 341:op_xor(a:(ReadLSB w32 (w32 0) unrolled__107), b:(ReadLSB w32 (w32 0) unrolled__109))
        u32 unrolled_3 = (unrolled_0) ^ (unrolled_2);
        // EP node  660829
        // BDD node 342:op_xor(a:(ReadLSB w32 (w32 0) unrolled__110), b:(ReadLSB w32 (w32 0) rotated__87))
        u32 unrolled_4 = (unrolled_3) ^ (bswap32(cpu_hdr_extra->rotated__87));
        // EP node  661290
        // BDD node 343:op_xor(a:(ReadLSB w32 (w32 0) rotated__86), b:(ReadLSB w32 (w32 0) unrolled__107))
        u32 unrolled_5 = (bswap32(cpu_hdr_extra->rotated__86)) ^ (unrolled_0);
        // EP node  661752
        // BDD node 344:op_xor(a:(ReadLSB w32 (w32 0) unrolled__111), b:(ReadLSB w32 (w32 0) unrolled__112))
        u32 unrolled_6 = (unrolled_4) ^ (unrolled_5);
        // EP node  662215
        // BDD node 345:op_xor(a:(ReadLSB w32 (w32 0) unrolled__105), b:(ReadLSB w32 (w32 0) unrolled__113))
        u32 unrolled_7 = (bswap32(cpu_hdr_extra->unrolled__105)) ^ (unrolled_6);
        // EP node  662679
        // BDD node 90:if ((Ule (Sub w32 (ReadLSB w32 (w32 0) unrolled__104) (ReadLSB w32 (w32 0) unrolled__114)) (w32 2))
        if (((bswap32(cpu_hdr_extra->unrolled__104)) - (unrolled_7)) <= (2)) {
          // EP node  662680
          // BDD node 90:if ((Ule (Sub w32 (ReadLSB w32 (w32 0) unrolled__104) (ReadLSB w32 (w32 0) unrolled__114)) (w32 2))
          // EP node  666421
          // BDD node 91:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073757136), l4_header:(w64 1073757392), packet:(w64 1073957384))
          trigger_update_ipv4_tcpudp_checksums = true;
          l3_hdr = (void *)hdr_1;
          l4_hdr = (void *)hdr_2;
          // EP node  667364
          // BDD node 92:packet_return_chunk(p:(w64 1074032664), the_chunk:(w64 1073757392)[(Concat w160 (Read w8 (w32 531) packet_chunks) (Concat w152 (Read w8 (w32 530) packet_chunks) (Concat w144 (Read w8 (w32 529) packet_chunks) (Concat w136 (Read w8 (w32 528) packet_chunks) (Concat w128 (Read w8 (w32 527) packet_chunks) (Concat w120 (Read w8 (w32 526) packet_chunks) (Concat w112 (Extract w8 0 (Or w32 (ZExt w32 (Read w8 (w32 525) packet_chunks)) (w32 64))) (Concat w104 (w8 80) (Concat w96 (Read w8 (w32 523) packet_chunks) (Concat w88 (Read w8 (w32 522) packet_chunks) (Concat w80 (Read w8 (w32 521) packet_chunks) (Concat w72 (Read w8 (w32 520) packet_chunks) (Concat w64 (Extract w8 0 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w56 (Extract w8 8 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w48 (Extract w8 16 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w40 (Extract w8 24 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (ReadLSB w32 (w32 512) packet_chunks)))))))))))))))))])
          hdr_2[4] = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4))))>>24);
          hdr_2[5] = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4))))>>16);
          hdr_2[6] = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4))))>>8);
          hdr_2[7] = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4)))));
          hdr_2[12] = 80;
          hdr_2[13] = (u8)((((u8)(*(u8*)(hdr_2 + 13))) | (64)));
          // EP node  668311
          // BDD node 93:packet_return_chunk(p:(w64 1074032664), the_chunk:(w64 1073757136)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (Read w8 (w32 271) packet_chunks) (Concat w120 (Read w8 (w32 270) packet_chunks) (Concat w112 (Read w8 (w32 269) packet_chunks) (Concat w104 (Read w8 (w32 268) packet_chunks) (Concat w96 (Read w8 (w32 1) checksum__91) (Concat w88 (Read w8 (w32 0) checksum__91) (Concat w80 (Read w8 (w32 265) packet_chunks) (Concat w72 (Read w8 (w32 264) packet_chunks) (Concat w64 (Read w8 (w32 263) packet_chunks) (Concat w56 (Read w8 (w32 262) packet_chunks) (Concat w48 (Read w8 (w32 261) packet_chunks) (Concat w40 (Read w8 (w32 260) packet_chunks) (Concat w32 (w8 40) (Concat w24 (w8 0) (Concat w16 (Read w8 (w32 257) packet_chunks) (w8 69))))))))))))))))))))])
          hdr_1[0] = 69;
          hdr_1[2] = 0;
          hdr_1[3] = 40;
          // EP node  670208
          // BDD node 95:FORWARD
          cpu_hdr->egress_dev = bswap16(0);
        } else {
          // EP node  662681
          // BDD node 90:if ((Ule (Sub w32 (ReadLSB w32 (w32 0) unrolled__104) (ReadLSB w32 (w32 0) unrolled__114)) (w32 2))
          // EP node  664547
          // BDD node 99:DROP
          result.forward = false;
        }
      } else {
        // EP node  595630
        // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
        // EP node  595631
        // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  595626
      // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
      // EP node  595627
      // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    }
  }
  else if (bswap16(cpu_hdr->code_path) == 1) {
    // EP node  651239
    // BDD node 211:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074066608)[ -> (w64 1073953400)])
    u8* hdr_3 = packet_consume(pkt, 14);
    // EP node  651240
    // BDD node 211:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074066608)[ -> (w64 1073953400)])
    u8* hdr_4 = packet_consume(pkt, 20);
    // EP node  651241
    // BDD node 211:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074066608)[ -> (w64 1073953400)])
    u8* hdr_5 = packet_consume(pkt, 8);
    // EP node  651242
    // BDD node 211:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074066608)[ -> (w64 1073953400)])
    u8* hdr_6 = packet_consume(pkt, 4);
    // EP node  665483
    // BDD node 211:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074066608)[ -> (w64 1073953400)])
    // EP node  666422
    // BDD node 458:op_lshr(a:(ReadLSB w64 (w32 0) next_time), b:(w64 16))
    u64 unrolled_8 = (u64)((now>>16) & 65535);
    // EP node  667837
    // BDD node 212:vector_return(vector:(w64 1073939504), index:(w32 0), value:(w64 1073953400)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__227) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    buffer_t vector_register_1073939504_value_0(4);
    vector_register_1073939504_value_0.set(0, 4, (unrolled_8 & 4294967295) - (bswap32(*(u32*)hdr_6)));
    state->vector_register_1073939504.put(0, vector_register_1073939504_value_0);
    // EP node  671159
    // BDD node 217:DROP
    result.forward = false;
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
