#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  BloomFilter bf_1073926928;
  VectorTable vector_table_1073939504;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      bf_1073926928("bf_1073926928",{"Ingress.bf_1073926928_row_0", "Ingress.bf_1073926928_row_1", }, 0LL),
      vector_table_1073939504("vector_table_1073939504",{"Ingress.vector_table_1073939504_105",})
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
  // Module DataplaneVectorTableAllocate

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 rotated__27;
  u32 unrolled__54;
  u32 unrolled__35;
  u64 unrolled__102;
  u32 bf_query_estimate__14;
  u32 rotated__31;
  u32 rotated__29;
  u32 unrolled__21;
  u32 rotated__30;
  u32 unrolled__15;
  u32 unrolled__105;
  u64 unrolled__227;

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



  if (bswap16(cpu_hdr->code_path) == 237392) {
    // EP node  237381
    // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  237382
    // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  237383
    // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
    u8* hdr_2 = packet_consume(pkt, 20);
    // EP node  237384
    // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
    if ((0) == (((u8)(*(u8*)(hdr_2 + 13))) & (2))) {
      // EP node  237385
      // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
      // EP node  237388
      // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
      if ((0) == (bswap32(cpu_hdr_extra->bf_query_estimate__14))) {
        // EP node  237389
        // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
        // EP node  277774
        // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
        buffer_t value_0;
        state->vector_table_1073939504.read(0, value_0);
        // EP node  278704
        // BDD node 89:vector_return(vector:(w64 1073939504), index:(w32 0), value:(w64 1073953400)[(ReadLSB w32 (w32 0) vector_data__88)])
        // EP node  279326
        // BDD node 250:op_xor(a:(ReadLSB w32 (w32 0) unrolled__15), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 272) packet_chunks) (Read w8 (w32 274) packet_chunks)) (Read w8 (w32 275) packet_chunks)))
        u32 unrolled_0 = (bswap32(cpu_hdr_extra->unrolled__15)) ^ (bswap32(*(u32*)(hdr_1 + 16)));
        // EP node  279950
        // BDD node 251:op_add(a:(ReadLSB w32 (w32 0) rotated__27), b:(ReadLSB w32 (w32 0) unrolled__19))
        u32 unrolled_1 = (bswap32(cpu_hdr_extra->rotated__27)) + (unrolled_0);
        // EP node  281515
        // BDD node 32:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__29) (ReadLSB w32 (w32 0) unrolled__20)), n:(w32 7))
        u32 rotated_0 = libnf::rotate_left((bswap32(cpu_hdr_extra->rotated__29)) ^ (unrolled_1), 7);
        // EP node  282771
        // BDD node 334:op_sub(a:(ReadLSB w32 (w32 0) unrolled__102), b:(ReadLSB w32 (w32 0) vector_data__88))
        u32 unrolled_2 = (bswap64(cpu_hdr_extra->unrolled__102) & 4294967295) - ((u32)value_0.get(0, 4));
        // EP node  284031
        // BDD node 33:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__20) (ReadLSB w32 (w32 0) unrolled__21)), n:(w32 16))
        u32 rotated_1 = libnf::rotate_left((unrolled_1) + (bswap32(cpu_hdr_extra->unrolled__21)), 16);
        // EP node  284979
        // BDD node 254:op_xor(a:(ReadLSB w32 (w32 0) rotated__29), b:(ReadLSB w32 (w32 0) unrolled__20))
        u32 unrolled_3 = (bswap32(cpu_hdr_extra->rotated__29)) ^ (unrolled_1);
        // EP node  285930
        // BDD node 253:op_add(a:(ReadLSB w32 (w32 0) unrolled__20), b:(ReadLSB w32 (w32 0) unrolled__21))
        u32 unrolled_4 = (unrolled_1) + (bswap32(cpu_hdr_extra->unrolled__21));
        // EP node  287202
        // BDD node 255:op_add(a:(ReadLSB w32 (w32 0) rotated__30), b:(ReadLSB w32 (w32 0) unrolled__23))
        u32 unrolled_5 = (bswap32(cpu_hdr_extra->rotated__30)) + (unrolled_3);
        // EP node  288797
        // BDD node 35:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__32) (ReadLSB w32 (w32 0) unrolled__24)), n:(w32 8))
        u32 rotated_2 = libnf::rotate_left((rotated_0) ^ (unrolled_5), 8);
        // EP node  290077
        // BDD node 34:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__31) (ReadLSB w32 (w32 0) unrolled__22)), n:(w32 5))
        u32 rotated_3 = libnf::rotate_left((bswap32(cpu_hdr_extra->rotated__31)) ^ (unrolled_4), 5);
        // EP node  291040
        // BDD node 256:op_xor(a:(ReadLSB w32 (w32 0) rotated__31), b:(ReadLSB w32 (w32 0) unrolled__22))
        u32 unrolled_6 = (bswap32(cpu_hdr_extra->rotated__31)) ^ (unrolled_4);
        // EP node  292328
        // BDD node 335:op_lshr(a:(ReadLSB w32 (w32 0) unrolled__103), b:(w32 12))
        u32 unrolled_7 = (unrolled_2) >> (12);
        // EP node  293297
        // BDD node 258:op_xor(a:(ReadLSB w32 (w32 0) rotated__32), b:(ReadLSB w32 (w32 0) unrolled__24))
        u32 unrolled_8 = (rotated_0) ^ (unrolled_5);
        // EP node  294269
        // BDD node 36:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__24) (ReadLSB w32 (w32 0) unrolled__25)), n:(w32 16))
        u32 rotated_4 = libnf::rotate_left((unrolled_5) + (unrolled_6), 16);
        // EP node  294919
        // BDD node 257:op_add(a:(ReadLSB w32 (w32 0) unrolled__24), b:(ReadLSB w32 (w32 0) unrolled__25))
        u32 unrolled_9 = (unrolled_5) + (unrolled_6);
        // EP node  295897
        // BDD node 259:op_add(a:(ReadLSB w32 (w32 0) rotated__33), b:(ReadLSB w32 (w32 0) unrolled__27))
        u32 unrolled_10 = (rotated_1) + (unrolled_8);
        // EP node  297205
        // BDD node 37:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__34) (ReadLSB w32 (w32 0) unrolled__26)), n:(w32 13))
        u32 rotated_5 = libnf::rotate_left((rotated_3) ^ (unrolled_9), 13);
        // EP node  298189
        // BDD node 260:op_xor(a:(ReadLSB w32 (w32 0) rotated__34), b:(ReadLSB w32 (w32 0) unrolled__26))
        u32 unrolled_11 = (rotated_3) ^ (unrolled_9);
        // EP node  299505
        // BDD node 39:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__28) (ReadLSB w32 (w32 0) unrolled__29)), n:(w32 16))
        u32 rotated_6 = libnf::rotate_left((unrolled_10) + (unrolled_11), 16);
        // EP node  300495
        // BDD node 261:op_add(a:(ReadLSB w32 (w32 0) unrolled__28), b:(ReadLSB w32 (w32 0) unrolled__29))
        u32 unrolled_12 = (unrolled_10) + (unrolled_11);
        // EP node  301819
        // BDD node 262:op_xor(a:(ReadLSB w32 (w32 0) rotated__35), b:(ReadLSB w32 (w32 0) unrolled__28))
        u32 unrolled_13 = (rotated_2) ^ (unrolled_10);
        // EP node  303147
        // BDD node 268:op_xor(a:(ReadLSB w32 (w32 0) rotated__37), b:(ReadLSB w32 (w32 0) unrolled__30))
        u32 unrolled_14 = (rotated_5) ^ (unrolled_12);
        // EP node  304146
        // BDD node 40:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__37) (ReadLSB w32 (w32 0) unrolled__30)), n:(w32 5))
        u32 rotated_7 = libnf::rotate_left((rotated_5) ^ (unrolled_12), 5);
        // EP node  304814
        // BDD node 263:op_add(a:(ReadLSB w32 (w32 0) rotated__36), b:(ReadLSB w32 (w32 0) unrolled__31))
        u32 unrolled_15 = (rotated_4) + (unrolled_13);
        // EP node  305484
        // BDD node 38:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__35) (ReadLSB w32 (w32 0) unrolled__28)), n:(w32 7))
        u32 rotated_8 = libnf::rotate_left((rotated_2) ^ (unrolled_10), 7);
        // EP node  306156
        // BDD node 264:op_xor(a:(ReadLSB w32 (w32 0) rotated__38), b:(ReadLSB w32 (w32 0) unrolled__32))
        u32 unrolled_16 = (rotated_8) ^ (unrolled_15);
        // EP node  307167
        // BDD node 41:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) unrolled__33) (ReadLSB w32 (w32 0) unrolled__35)), n:(w32 8))
        u32 rotated_9 = libnf::rotate_left((unrolled_16) ^ (bswap32(cpu_hdr_extra->unrolled__35)), 8);
        // EP node  307843
        // BDD node 267:op_xor(a:(ReadLSB w32 (w32 0) unrolled__32), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 272) packet_chunks) (Read w8 (w32 274) packet_chunks)) (Read w8 (w32 275) packet_chunks)))
        u32 unrolled_17 = (unrolled_15) ^ (bswap32(*(u32*)(hdr_1 + 16)));
        // EP node  308860
        // BDD node 42:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__36) (ReadLSB w32 (w32 0) unrolled__37)), n:(w32 16))
        u32 rotated_10 = libnf::rotate_left((unrolled_17) + (unrolled_14), 16);
        // EP node  309540
        // BDD node 269:op_add(a:(ReadLSB w32 (w32 0) unrolled__36), b:(ReadLSB w32 (w32 0) unrolled__37))
        u32 unrolled_18 = (unrolled_17) + (unrolled_14);
        // EP node  310563
        // BDD node 43:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__40) (ReadLSB w32 (w32 0) unrolled__38)), n:(w32 13))
        u32 rotated_11 = libnf::rotate_left((rotated_7) ^ (unrolled_18), 13);
        // EP node  311247
        // BDD node 272:op_xor(a:(ReadLSB w32 (w32 0) rotated__40), b:(ReadLSB w32 (w32 0) unrolled__38))
        u32 unrolled_19 = (rotated_7) ^ (unrolled_18);
        // EP node  311590
        // BDD node 270:op_xor(a:(ReadLSB w32 (w32 0) unrolled__33), b:(ReadLSB w32 (w32 0) unrolled__35))
        u32 unrolled_20 = (unrolled_16) ^ (bswap32(cpu_hdr_extra->unrolled__35));
        // EP node  311934
        // BDD node 271:op_add(a:(ReadLSB w32 (w32 0) rotated__39), b:(ReadLSB w32 (w32 0) unrolled__39))
        u32 unrolled_21 = (rotated_6) + (unrolled_20);
        // EP node  313314
        // BDD node 274:op_xor(a:(ReadLSB w32 (w32 0) rotated__41), b:(ReadLSB w32 (w32 0) unrolled__40))
        u32 unrolled_22 = (rotated_9) ^ (unrolled_21);
        // EP node  314698
        // BDD node 275:op_add(a:(ReadLSB w32 (w32 0) rotated__42), b:(ReadLSB w32 (w32 0) unrolled__43))
        u32 unrolled_23 = (rotated_10) + (unrolled_22);
        // EP node  315739
        // BDD node 44:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__41) (ReadLSB w32 (w32 0) unrolled__40)), n:(w32 7))
        u32 rotated_12 = libnf::rotate_left((rotated_9) ^ (unrolled_21), 7);
        // EP node  317131
        // BDD node 47:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__44) (ReadLSB w32 (w32 0) unrolled__44)), n:(w32 8))
        u32 rotated_13 = libnf::rotate_left((rotated_12) ^ (unrolled_23), 8);
        // EP node  318178
        // BDD node 45:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__40) (ReadLSB w32 (w32 0) unrolled__41)), n:(w32 16))
        u32 rotated_14 = libnf::rotate_left((unrolled_21) + (unrolled_19), 16);
        // EP node  318878
        // BDD node 273:op_add(a:(ReadLSB w32 (w32 0) unrolled__40), b:(ReadLSB w32 (w32 0) unrolled__41))
        u32 unrolled_24 = (unrolled_21) + (unrolled_19);
        // EP node  319931
        // BDD node 46:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__43) (ReadLSB w32 (w32 0) unrolled__42)), n:(w32 5))
        u32 rotated_15 = libnf::rotate_left((rotated_11) ^ (unrolled_24), 5);
        // EP node  320635
        // BDD node 278:op_xor(a:(ReadLSB w32 (w32 0) rotated__44), b:(ReadLSB w32 (w32 0) unrolled__44))
        u32 unrolled_25 = (rotated_12) ^ (unrolled_23);
        // EP node  321341
        // BDD node 276:op_xor(a:(ReadLSB w32 (w32 0) rotated__43), b:(ReadLSB w32 (w32 0) unrolled__42))
        u32 unrolled_26 = (rotated_11) ^ (unrolled_24);
        // EP node  322403
        // BDD node 48:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__44) (ReadLSB w32 (w32 0) unrolled__45)), n:(w32 16))
        u32 rotated_16 = libnf::rotate_left((unrolled_23) + (unrolled_26), 16);
        // EP node  323113
        // BDD node 277:op_add(a:(ReadLSB w32 (w32 0) unrolled__44), b:(ReadLSB w32 (w32 0) unrolled__45))
        u32 unrolled_27 = (unrolled_23) + (unrolled_26);
        // EP node  324181
        // BDD node 280:op_xor(a:(ReadLSB w32 (w32 0) rotated__46), b:(ReadLSB w32 (w32 0) unrolled__46))
        u32 unrolled_28 = (rotated_15) ^ (unrolled_27);
        // EP node  324895
        // BDD node 279:op_add(a:(ReadLSB w32 (w32 0) rotated__45), b:(ReadLSB w32 (w32 0) unrolled__47))
        u32 unrolled_29 = (rotated_14) + (unrolled_25);
        // EP node  326685
        // BDD node 50:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__47) (ReadLSB w32 (w32 0) unrolled__48)), n:(w32 7))
        u32 rotated_17 = libnf::rotate_left((rotated_13) ^ (unrolled_29), 7);
        // EP node  328121
        // BDD node 281:op_add(a:(ReadLSB w32 (w32 0) unrolled__48), b:(ReadLSB w32 (w32 0) unrolled__49))
        u32 unrolled_30 = (unrolled_29) + (unrolled_28);
        // EP node  329201
        // BDD node 282:op_xor(a:(ReadLSB w32 (w32 0) rotated__47), b:(ReadLSB w32 (w32 0) unrolled__48))
        u32 unrolled_31 = (rotated_13) ^ (unrolled_29);
        // EP node  330284
        // BDD node 283:op_add(a:(ReadLSB w32 (w32 0) rotated__48), b:(ReadLSB w32 (w32 0) unrolled__51))
        u32 unrolled_32 = (rotated_16) + (unrolled_31);
        // EP node  331732
        // BDD node 49:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__46) (ReadLSB w32 (w32 0) unrolled__46)), n:(w32 13))
        u32 rotated_18 = libnf::rotate_left((rotated_15) ^ (unrolled_27), 13);
        // EP node  333547
        // BDD node 51:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__48) (ReadLSB w32 (w32 0) unrolled__49)), n:(w32 16))
        u32 rotated_19 = libnf::rotate_left((unrolled_29) + (unrolled_28), 16);
        // EP node  335003
        // BDD node 52:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__49) (ReadLSB w32 (w32 0) unrolled__50)), n:(w32 5))
        u32 rotated_20 = libnf::rotate_left((rotated_18) ^ (unrolled_30), 5);
        // EP node  336098
        // BDD node 284:op_xor(a:(ReadLSB w32 (w32 0) rotated__50), b:(ReadLSB w32 (w32 0) unrolled__52))
        u32 unrolled_33 = (rotated_17) ^ (unrolled_32);
        // EP node  337562
        // BDD node 286:op_xor(a:(ReadLSB w32 (w32 0) unrolled__52), b:(ReadLSB w32 (w32 0) unrolled__35))
        u32 unrolled_34 = (unrolled_32) ^ (bswap32(cpu_hdr_extra->unrolled__35));
        // EP node  338663
        // BDD node 53:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) unrolled__53) (ReadLSB w32 (w32 0) unrolled__54)), n:(w32 8))
        u32 rotated_21 = libnf::rotate_left((unrolled_33) ^ (bswap32(cpu_hdr_extra->unrolled__54)), 8);
        // EP node  339399
        // BDD node 287:op_xor(a:(ReadLSB w32 (w32 0) rotated__49), b:(ReadLSB w32 (w32 0) unrolled__50))
        u32 unrolled_35 = (rotated_18) ^ (unrolled_30);
        // EP node  340506
        // BDD node 54:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__55) (ReadLSB w32 (w32 0) unrolled__56)), n:(w32 16))
        u32 rotated_22 = libnf::rotate_left((unrolled_34) + (unrolled_35), 16);
        // EP node  341246
        // BDD node 288:op_add(a:(ReadLSB w32 (w32 0) unrolled__55), b:(ReadLSB w32 (w32 0) unrolled__56))
        u32 unrolled_36 = (unrolled_34) + (unrolled_35);
        // EP node  342359
        // BDD node 55:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__52) (ReadLSB w32 (w32 0) unrolled__57)), n:(w32 13))
        u32 rotated_23 = libnf::rotate_left((rotated_20) ^ (unrolled_36), 13);
        // EP node  343103
        // BDD node 289:op_xor(a:(ReadLSB w32 (w32 0) unrolled__53), b:(ReadLSB w32 (w32 0) unrolled__54))
        u32 unrolled_37 = (unrolled_33) ^ (bswap32(cpu_hdr_extra->unrolled__54));
        // EP node  343849
        // BDD node 291:op_xor(a:(ReadLSB w32 (w32 0) rotated__52), b:(ReadLSB w32 (w32 0) unrolled__57))
        u32 unrolled_38 = (rotated_20) ^ (unrolled_36);
        // EP node  344223
        // BDD node 290:op_add(a:(ReadLSB w32 (w32 0) rotated__51), b:(ReadLSB w32 (w32 0) unrolled__58))
        u32 unrolled_39 = (rotated_19) + (unrolled_37);
        // EP node  345723
        // BDD node 56:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__53) (ReadLSB w32 (w32 0) unrolled__59)), n:(w32 7))
        u32 rotated_24 = libnf::rotate_left((rotated_21) ^ (unrolled_39), 7);
        // EP node  346851
        // BDD node 57:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__59) (ReadLSB w32 (w32 0) unrolled__60)), n:(w32 16))
        u32 rotated_25 = libnf::rotate_left((unrolled_39) + (unrolled_38), 16);
        // EP node  347605
        // BDD node 292:op_add(a:(ReadLSB w32 (w32 0) unrolled__59), b:(ReadLSB w32 (w32 0) unrolled__60))
        u32 unrolled_40 = (unrolled_39) + (unrolled_38);
        // EP node  348739
        // BDD node 295:op_xor(a:(ReadLSB w32 (w32 0) rotated__55), b:(ReadLSB w32 (w32 0) unrolled__61))
        u32 unrolled_41 = (rotated_23) ^ (unrolled_40);
        // EP node  349497
        // BDD node 58:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__55) (ReadLSB w32 (w32 0) unrolled__61)), n:(w32 5))
        u32 rotated_26 = libnf::rotate_left((rotated_23) ^ (unrolled_40), 5);
        // EP node  349877
        // BDD node 293:op_xor(a:(ReadLSB w32 (w32 0) rotated__53), b:(ReadLSB w32 (w32 0) unrolled__59))
        u32 unrolled_42 = (rotated_21) ^ (unrolled_39);
        // EP node  350258
        // BDD node 294:op_add(a:(ReadLSB w32 (w32 0) rotated__54), b:(ReadLSB w32 (w32 0) unrolled__62))
        u32 unrolled_43 = (rotated_22) + (unrolled_42);
        // EP node  351786
        // BDD node 296:op_add(a:(ReadLSB w32 (w32 0) unrolled__63), b:(ReadLSB w32 (w32 0) unrolled__64))
        u32 unrolled_44 = (unrolled_43) + (unrolled_41);
        // EP node  353701
        // BDD node 60:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__63) (ReadLSB w32 (w32 0) unrolled__64)), n:(w32 16))
        u32 rotated_27 = libnf::rotate_left((unrolled_43) + (unrolled_41), 16);
        // EP node  355237
        // BDD node 59:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__56) (ReadLSB w32 (w32 0) unrolled__63)), n:(w32 8))
        u32 rotated_28 = libnf::rotate_left((rotated_24) ^ (unrolled_43), 8);
        // EP node  356392
        // BDD node 299:op_xor(a:(ReadLSB w32 (w32 0) rotated__58), b:(ReadLSB w32 (w32 0) unrolled__65))
        u32 unrolled_45 = (rotated_26) ^ (unrolled_44);
        // EP node  357164
        // BDD node 61:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__58) (ReadLSB w32 (w32 0) unrolled__65)), n:(w32 13))
        u32 rotated_29 = libnf::rotate_left((rotated_26) ^ (unrolled_44), 13);
        // EP node  357551
        // BDD node 297:op_xor(a:(ReadLSB w32 (w32 0) rotated__56), b:(ReadLSB w32 (w32 0) unrolled__63))
        u32 unrolled_46 = (rotated_24) ^ (unrolled_43);
        // EP node  357939
        // BDD node 298:op_add(a:(ReadLSB w32 (w32 0) rotated__57), b:(ReadLSB w32 (w32 0) unrolled__66))
        u32 unrolled_47 = (rotated_25) + (unrolled_46);
        // EP node  359495
        // BDD node 62:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__59) (ReadLSB w32 (w32 0) unrolled__67)), n:(w32 7))
        u32 rotated_30 = libnf::rotate_left((rotated_28) ^ (unrolled_47), 7);
        // EP node  360665
        // BDD node 63:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__67) (ReadLSB w32 (w32 0) unrolled__68)), n:(w32 16))
        u32 rotated_31 = libnf::rotate_left((unrolled_47) + (unrolled_45), 16);
        // EP node  361447
        // BDD node 300:op_add(a:(ReadLSB w32 (w32 0) unrolled__67), b:(ReadLSB w32 (w32 0) unrolled__68))
        u32 unrolled_48 = (unrolled_47) + (unrolled_45);
        // EP node  362623
        // BDD node 64:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__61) (ReadLSB w32 (w32 0) unrolled__69)), n:(w32 5))
        u32 rotated_32 = libnf::rotate_left((rotated_29) ^ (unrolled_48), 5);
        // EP node  363409
        // BDD node 301:op_xor(a:(ReadLSB w32 (w32 0) rotated__59), b:(ReadLSB w32 (w32 0) unrolled__67))
        u32 unrolled_49 = (rotated_28) ^ (unrolled_47);
        // EP node  364197
        // BDD node 304:op_xor(a:(ReadLSB w32 (w32 0) rotated__61), b:(ReadLSB w32 (w32 0) unrolled__69))
        u32 unrolled_50 = (rotated_29) ^ (unrolled_48);
        // EP node  364592
        // BDD node 302:op_add(a:(ReadLSB w32 (w32 0) rotated__60), b:(ReadLSB w32 (w32 0) unrolled__70))
        u32 unrolled_51 = (rotated_27) + (unrolled_49);
        // EP node  365780
        // BDD node 65:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__62) (ReadLSB w32 (w32 0) unrolled__71)), n:(w32 8))
        u32 rotated_33 = libnf::rotate_left((rotated_30) ^ (unrolled_51), 8);
        // EP node  366574
        // BDD node 306:op_xor(a:(ReadLSB w32 (w32 0) rotated__62), b:(ReadLSB w32 (w32 0) unrolled__71))
        u32 unrolled_52 = (rotated_30) ^ (unrolled_51);
        // EP node  367370
        // BDD node 307:op_add(a:(ReadLSB w32 (w32 0) rotated__63), b:(ReadLSB w32 (w32 0) unrolled__75))
        u32 unrolled_53 = (rotated_31) + (unrolled_52);
        // EP node  368567
        // BDD node 68:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__65) (ReadLSB w32 (w32 0) unrolled__76)), n:(w32 7))
        u32 rotated_34 = libnf::rotate_left((rotated_33) ^ (unrolled_53), 7);
        // EP node  369367
        // BDD node 303:op_xor(a:(ReadLSB w32 (w32 0) unrolled__71), b:(ReadLSB w32 (w32 0) unrolled__54))
        u32 unrolled_54 = (unrolled_51) ^ (bswap32(cpu_hdr_extra->unrolled__54));
        // EP node  370570
        // BDD node 310:op_xor(a:(ReadLSB w32 (w32 0) rotated__65), b:(ReadLSB w32 (w32 0) unrolled__76))
        u32 unrolled_55 = (rotated_33) ^ (unrolled_53);
        // EP node  371374
        // BDD node 305:op_add(a:(ReadLSB w32 (w32 0) unrolled__72), b:(ReadLSB w32 (w32 0) unrolled__73))
        u32 unrolled_56 = (unrolled_54) + (unrolled_50);
        // EP node  372583
        // BDD node 66:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__72) (ReadLSB w32 (w32 0) unrolled__73)), n:(w32 16))
        u32 rotated_35 = libnf::rotate_left((unrolled_54) + (unrolled_50), 16);
        // EP node  373795
        // BDD node 67:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__64) (ReadLSB w32 (w32 0) unrolled__74)), n:(w32 13))
        u32 rotated_36 = libnf::rotate_left((rotated_32) ^ (unrolled_56), 13);
        // EP node  374605
        // BDD node 308:op_xor(a:(ReadLSB w32 (w32 0) rotated__64), b:(ReadLSB w32 (w32 0) unrolled__74))
        u32 unrolled_57 = (rotated_32) ^ (unrolled_56);
        // EP node  375823
        // BDD node 309:op_add(a:(ReadLSB w32 (w32 0) unrolled__76), b:(ReadLSB w32 (w32 0) unrolled__77))
        u32 unrolled_58 = (unrolled_53) + (unrolled_57);
        // EP node  377451
        // BDD node 69:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__76) (ReadLSB w32 (w32 0) unrolled__77)), n:(w32 16))
        u32 rotated_37 = libnf::rotate_left((unrolled_53) + (unrolled_57), 16);
        // EP node  378675
        // BDD node 70:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__67) (ReadLSB w32 (w32 0) unrolled__78)), n:(w32 5))
        u32 rotated_38 = libnf::rotate_left((rotated_36) ^ (unrolled_58), 5);
        // EP node  379493
        // BDD node 311:op_add(a:(ReadLSB w32 (w32 0) rotated__66), b:(ReadLSB w32 (w32 0) unrolled__79))
        u32 unrolled_59 = (rotated_35) + (unrolled_55);
        // EP node  380723
        // BDD node 71:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__68) (ReadLSB w32 (w32 0) unrolled__80)), n:(w32 8))
        u32 rotated_39 = libnf::rotate_left((rotated_34) ^ (unrolled_59), 8);
        // EP node  381545
        // BDD node 312:op_xor(a:(ReadLSB w32 (w32 0) rotated__67), b:(ReadLSB w32 (w32 0) unrolled__78))
        u32 unrolled_60 = (rotated_36) ^ (unrolled_58);
        // EP node  382781
        // BDD node 72:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__80) (ReadLSB w32 (w32 0) unrolled__81)), n:(w32 16))
        u32 rotated_40 = libnf::rotate_left((unrolled_59) + (unrolled_60), 16);
        // EP node  383607
        // BDD node 314:op_xor(a:(ReadLSB w32 (w32 0) rotated__68), b:(ReadLSB w32 (w32 0) unrolled__80))
        u32 unrolled_61 = (rotated_34) ^ (unrolled_59);
        // EP node  384435
        // BDD node 313:op_add(a:(ReadLSB w32 (w32 0) unrolled__80), b:(ReadLSB w32 (w32 0) unrolled__81))
        u32 unrolled_62 = (unrolled_59) + (unrolled_60);
        // EP node  385680
        // BDD node 316:op_xor(a:(ReadLSB w32 (w32 0) rotated__70), b:(ReadLSB w32 (w32 0) unrolled__82))
        u32 unrolled_63 = (rotated_38) ^ (unrolled_62);
        // EP node  386512
        // BDD node 73:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__70) (ReadLSB w32 (w32 0) unrolled__82)), n:(w32 13))
        u32 rotated_41 = libnf::rotate_left((rotated_38) ^ (unrolled_62), 13);
        // EP node  386929
        // BDD node 315:op_add(a:(ReadLSB w32 (w32 0) rotated__69), b:(ReadLSB w32 (w32 0) unrolled__83))
        u32 unrolled_64 = (rotated_37) + (unrolled_61);
        // EP node  388601
        // BDD node 318:op_xor(a:(ReadLSB w32 (w32 0) rotated__71), b:(ReadLSB w32 (w32 0) unrolled__84))
        u32 unrolled_65 = (rotated_39) ^ (unrolled_64);
        // EP node  390277
        // BDD node 75:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__84) (ReadLSB w32 (w32 0) unrolled__85)), n:(w32 16))
        u32 rotated_42 = libnf::rotate_left((unrolled_64) + (unrolled_63), 16);
        // EP node  391537
        // BDD node 74:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__71) (ReadLSB w32 (w32 0) unrolled__84)), n:(w32 7))
        u32 rotated_43 = libnf::rotate_left((rotated_39) ^ (unrolled_64), 7);
        // EP node  392379
        // BDD node 317:op_add(a:(ReadLSB w32 (w32 0) unrolled__84), b:(ReadLSB w32 (w32 0) unrolled__85))
        u32 unrolled_66 = (unrolled_64) + (unrolled_63);
        // EP node  393645
        // BDD node 319:op_add(a:(ReadLSB w32 (w32 0) rotated__72), b:(ReadLSB w32 (w32 0) unrolled__87))
        u32 unrolled_67 = (rotated_40) + (unrolled_65);
        // EP node  395337
        // BDD node 320:op_xor(a:(ReadLSB w32 (w32 0) rotated__73), b:(ReadLSB w32 (w32 0) unrolled__86))
        u32 unrolled_68 = (rotated_41) ^ (unrolled_66);
        // EP node  397457
        // BDD node 76:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__73) (ReadLSB w32 (w32 0) unrolled__86)), n:(w32 5))
        u32 rotated_44 = libnf::rotate_left((rotated_41) ^ (unrolled_66), 5);
        // EP node  399157
        // BDD node 78:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__88) (ReadLSB w32 (w32 0) unrolled__89)), n:(w32 16))
        u32 rotated_45 = libnf::rotate_left((unrolled_67) + (unrolled_68), 16);
        // EP node  400435
        // BDD node 321:op_add(a:(ReadLSB w32 (w32 0) unrolled__88), b:(ReadLSB w32 (w32 0) unrolled__89))
        u32 unrolled_69 = (unrolled_67) + (unrolled_68);
        // EP node  402143
        // BDD node 77:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__74) (ReadLSB w32 (w32 0) unrolled__88)), n:(w32 8))
        u32 rotated_46 = libnf::rotate_left((rotated_43) ^ (unrolled_67), 8);
        // EP node  403427
        // BDD node 79:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__76) (ReadLSB w32 (w32 0) unrolled__90)), n:(w32 13))
        u32 rotated_47 = libnf::rotate_left((rotated_44) ^ (unrolled_69), 13);
        // EP node  404285
        // BDD node 322:op_xor(a:(ReadLSB w32 (w32 0) rotated__74), b:(ReadLSB w32 (w32 0) unrolled__88))
        u32 unrolled_70 = (rotated_43) ^ (unrolled_67);
        // EP node  405145
        // BDD node 324:op_xor(a:(ReadLSB w32 (w32 0) rotated__76), b:(ReadLSB w32 (w32 0) unrolled__90))
        u32 unrolled_71 = (rotated_44) ^ (unrolled_69);
        // EP node  405576
        // BDD node 323:op_add(a:(ReadLSB w32 (w32 0) rotated__75), b:(ReadLSB w32 (w32 0) unrolled__91))
        u32 unrolled_72 = (rotated_42) + (unrolled_70);
        // EP node  407304
        // BDD node 81:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__92) (ReadLSB w32 (w32 0) unrolled__93)), n:(w32 16))
        u32 rotated_48 = libnf::rotate_left((unrolled_72) + (unrolled_71), 16);
        // EP node  408603
        // BDD node 325:op_add(a:(ReadLSB w32 (w32 0) unrolled__92), b:(ReadLSB w32 (w32 0) unrolled__93))
        u32 unrolled_73 = (unrolled_72) + (unrolled_71);
        // EP node  410339
        // BDD node 80:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__77) (ReadLSB w32 (w32 0) unrolled__92)), n:(w32 7))
        u32 rotated_49 = libnf::rotate_left((rotated_46) ^ (unrolled_72), 7);
        // EP node  411644
        // BDD node 82:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__79) (ReadLSB w32 (w32 0) unrolled__94)), n:(w32 5))
        u32 rotated_50 = libnf::rotate_left((rotated_47) ^ (unrolled_73), 5);
        // EP node  412516
        // BDD node 328:op_xor(a:(ReadLSB w32 (w32 0) rotated__79), b:(ReadLSB w32 (w32 0) unrolled__94))
        u32 unrolled_74 = (rotated_47) ^ (unrolled_73);
        // EP node  412953
        // BDD node 326:op_xor(a:(ReadLSB w32 (w32 0) rotated__77), b:(ReadLSB w32 (w32 0) unrolled__92))
        u32 unrolled_75 = (rotated_46) ^ (unrolled_72);
        // EP node  413391
        // BDD node 327:op_add(a:(ReadLSB w32 (w32 0) rotated__78), b:(ReadLSB w32 (w32 0) unrolled__95))
        u32 unrolled_76 = (rotated_45) + (unrolled_75);
        // EP node  415147
        // BDD node 83:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__80) (ReadLSB w32 (w32 0) unrolled__96)), n:(w32 8))
        u32 rotated_51 = libnf::rotate_left((rotated_49) ^ (unrolled_76), 8);
        // EP node  416467
        // BDD node 84:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__96) (ReadLSB w32 (w32 0) unrolled__97)), n:(w32 16))
        u32 rotated_52 = libnf::rotate_left((unrolled_76) + (unrolled_74), 16);
        // EP node  417349
        // BDD node 329:op_add(a:(ReadLSB w32 (w32 0) unrolled__96), b:(ReadLSB w32 (w32 0) unrolled__97))
        u32 unrolled_77 = (unrolled_76) + (unrolled_74);
        // EP node  418675
        // BDD node 85:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__82) (ReadLSB w32 (w32 0) unrolled__98)), n:(w32 13))
        u32 rotated_53 = libnf::rotate_left((rotated_50) ^ (unrolled_77), 13);
        // EP node  419561
        // BDD node 330:op_xor(a:(ReadLSB w32 (w32 0) rotated__80), b:(ReadLSB w32 (w32 0) unrolled__96))
        u32 unrolled_78 = (rotated_49) ^ (unrolled_76);
        // EP node  420449
        // BDD node 332:op_xor(a:(ReadLSB w32 (w32 0) rotated__82), b:(ReadLSB w32 (w32 0) unrolled__98))
        u32 unrolled_79 = (rotated_50) ^ (unrolled_77);
        // EP node  420894
        // BDD node 331:op_add(a:(ReadLSB w32 (w32 0) rotated__81), b:(ReadLSB w32 (w32 0) unrolled__99))
        u32 unrolled_80 = (rotated_48) + (unrolled_78);
        // EP node  422678
        // BDD node 86:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__83) (ReadLSB w32 (w32 0) unrolled__100)), n:(w32 7))
        u32 rotated_54 = libnf::rotate_left((rotated_51) ^ (unrolled_80), 7);
        // EP node  424019
        // BDD node 337:op_xor(a:(ReadLSB w32 (w32 0) rotated__83), b:(ReadLSB w32 (w32 0) unrolled__100))
        u32 unrolled_81 = (rotated_51) ^ (unrolled_80);
        // EP node  425363
        // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__106))
        u32 unrolled_82 = (rotated_52) + (unrolled_81);
        // EP node  426710
        // BDD node 343:op_xor(a:(ReadLSB w32 (w32 0) rotated__86), b:(ReadLSB w32 (w32 0) unrolled__107))
        u32 unrolled_83 = (rotated_54) ^ (unrolled_82);
        // EP node  427610
        // BDD node 87:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__100) (ReadLSB w32 (w32 0) unrolled__101)), n:(w32 16))
        u32 rotated_55 = libnf::rotate_left((unrolled_80) + (unrolled_79), 16);
        // EP node  428061
        // BDD node 339:op_add(a:(ReadLSB w32 (w32 0) unrolled__100), b:(ReadLSB w32 (w32 0) unrolled__101))
        u32 unrolled_84 = (unrolled_80) + (unrolled_79);
        // EP node  428513
        // BDD node 340:op_xor(a:(ReadLSB w32 (w32 0) rotated__85), b:(ReadLSB w32 (w32 0) unrolled__108))
        u32 unrolled_85 = (rotated_53) ^ (unrolled_84);
        // EP node  428966
        // BDD node 341:op_xor(a:(ReadLSB w32 (w32 0) unrolled__107), b:(ReadLSB w32 (w32 0) unrolled__109))
        u32 unrolled_86 = (unrolled_82) ^ (unrolled_85);
        // EP node  429420
        // BDD node 342:op_xor(a:(ReadLSB w32 (w32 0) unrolled__110), b:(ReadLSB w32 (w32 0) rotated__87))
        u32 unrolled_87 = (unrolled_86) ^ (rotated_55);
        // EP node  429875
        // BDD node 344:op_xor(a:(ReadLSB w32 (w32 0) unrolled__111), b:(ReadLSB w32 (w32 0) unrolled__112))
        u32 unrolled_88 = (unrolled_87) ^ (unrolled_83);
        // EP node  430331
        // BDD node 345:op_xor(a:(ReadLSB w32 (w32 0) unrolled__105), b:(ReadLSB w32 (w32 0) unrolled__113))
        u32 unrolled_89 = (bswap32(cpu_hdr_extra->unrolled__105)) ^ (unrolled_88);
        // EP node  430788
        // BDD node 90:if ((Ule (Sub w32 (ReadLSB w32 (w32 0) unrolled__104) (ReadLSB w32 (w32 0) unrolled__114)) (w32 2))
        if (((unrolled_7) - (unrolled_89)) <= (2)) {
          // EP node  430789
          // BDD node 90:if ((Ule (Sub w32 (ReadLSB w32 (w32 0) unrolled__104) (ReadLSB w32 (w32 0) unrolled__114)) (w32 2))
          // EP node  434013
          // BDD node 91:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073757136), l4_header:(w64 1073757392), packet:(w64 1073957384))
          trigger_update_ipv4_tcpudp_checksums = true;
          l3_hdr = (void *)hdr_1;
          l4_hdr = (void *)hdr_2;
          // EP node  434940
          // BDD node 92:packet_return_chunk(p:(w64 1074032664), the_chunk:(w64 1073757392)[(Concat w160 (Read w8 (w32 531) packet_chunks) (Concat w152 (Read w8 (w32 530) packet_chunks) (Concat w144 (Read w8 (w32 529) packet_chunks) (Concat w136 (Read w8 (w32 528) packet_chunks) (Concat w128 (Read w8 (w32 527) packet_chunks) (Concat w120 (Read w8 (w32 526) packet_chunks) (Concat w112 (Extract w8 0 (Or w32 (ZExt w32 (Read w8 (w32 525) packet_chunks)) (w32 64))) (Concat w104 (w8 80) (Concat w96 (Read w8 (w32 523) packet_chunks) (Concat w88 (Read w8 (w32 522) packet_chunks) (Concat w80 (Read w8 (w32 521) packet_chunks) (Concat w72 (Read w8 (w32 520) packet_chunks) (Concat w64 (Extract w8 0 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w56 (Extract w8 8 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w48 (Extract w8 16 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w40 (Extract w8 24 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (ReadLSB w32 (w32 512) packet_chunks)))))))))))))))))])
          hdr_2[4] = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4))))>>24);
          hdr_2[5] = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4))))>>16);
          hdr_2[6] = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4))))>>8);
          hdr_2[7] = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4)))));
          hdr_2[12] = 80;
          hdr_2[13] = (u8)((((u8)(*(u8*)(hdr_2 + 13))) | (64)));
          // EP node  435869
          // BDD node 93:packet_return_chunk(p:(w64 1074032664), the_chunk:(w64 1073757136)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (Read w8 (w32 271) packet_chunks) (Concat w120 (Read w8 (w32 270) packet_chunks) (Concat w112 (Read w8 (w32 269) packet_chunks) (Concat w104 (Read w8 (w32 268) packet_chunks) (Concat w96 (Read w8 (w32 1) checksum__91) (Concat w88 (Read w8 (w32 0) checksum__91) (Concat w80 (Read w8 (w32 265) packet_chunks) (Concat w72 (Read w8 (w32 264) packet_chunks) (Concat w64 (Read w8 (w32 263) packet_chunks) (Concat w56 (Read w8 (w32 262) packet_chunks) (Concat w48 (Read w8 (w32 261) packet_chunks) (Concat w40 (Read w8 (w32 260) packet_chunks) (Concat w32 (w8 40) (Concat w24 (w8 0) (Concat w16 (Read w8 (w32 257) packet_chunks) (w8 69))))))))))))))))))))])
          hdr_1[0] = 69;
          hdr_1[2] = 0;
          hdr_1[3] = 40;
          // EP node  437730
          // BDD node 95:FORWARD
          cpu_hdr->egress_dev = bswap16(0);
        } else {
          // EP node  430790
          // BDD node 90:if ((Ule (Sub w32 (ReadLSB w32 (w32 0) unrolled__104) (ReadLSB w32 (w32 0) unrolled__114)) (w32 2))
          // EP node  432628
          // BDD node 99:DROP
          result.forward = false;
        }
      } else {
        // EP node  237390
        // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
        // EP node  237391
        // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  237386
      // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
      // EP node  237387
      // BDD node 88:vector_borrow(vector:(w64 1073939504), index:(w32 0), val_out:(w64 1074050240)[ -> (w64 1073953400)])
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    }
  }
  else if (bswap16(cpu_hdr->code_path) == 276230) {
    // EP node  276226
    // BDD node 212:vector_return(vector:(w64 1073939504), index:(w32 0), value:(w64 1073953400)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__227) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_3 = packet_consume(pkt, 14);
    // EP node  276227
    // BDD node 212:vector_return(vector:(w64 1073939504), index:(w32 0), value:(w64 1073953400)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__227) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_4 = packet_consume(pkt, 20);
    // EP node  276228
    // BDD node 212:vector_return(vector:(w64 1073939504), index:(w32 0), value:(w64 1073953400)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__227) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_5 = packet_consume(pkt, 8);
    // EP node  276229
    // BDD node 212:vector_return(vector:(w64 1073939504), index:(w32 0), value:(w64 1073953400)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__227) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_6 = packet_consume(pkt, 4);
    // EP node  433089
    // BDD node 212:vector_return(vector:(w64 1073939504), index:(w32 0), value:(w64 1073953400)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__227) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    buffer_t vector_table_1073939504_value_0(4);
    vector_table_1073939504_value_0.set(0, 4, (bswap64(cpu_hdr_extra->unrolled__227) & 4294967295) - (bswap32(*(u32*)hdr_6)));
    state->vector_table_1073939504.write(0, vector_table_1073939504_value_0);
    // EP node  437731
    // BDD node 217:DROP
    result.forward = false;
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
