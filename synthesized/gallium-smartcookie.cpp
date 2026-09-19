#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  BloomFilter bf_1073927040;
  VectorRegister vector_register_1073939616;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      bf_1073927040("bf_1073927040",{"Ingress.bf_1073927040_row_0", "Ingress.bf_1073927040_row_1", }, 0LL),
      vector_register_1073939616("vector_register_1073939616",{"Ingress.vector_register_1073939616_0",})
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
  // BDD node 0:bf_allocate(height:(w32 2), width:(w32 1048576), key_size:(w16 12), cleanup_interval:(w64 0), bf_out:(w64 1073926768)[(w64 0) -> (w64 1073927040)])
  // Module DataplaneBloomFilterAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 4), capacity:(w32 1), vector_out:(w64 1073926776)[(w64 0) -> (w64 1073939616)])
  // Module DataplaneVectorRegisterAllocate

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 time; // The switch's clock at the hand-off, ingress_mac_tstamp[47:16].
  u32 bf_query_estimate__14;
  u32 DEVICE;
  u32 vector_data__105;
  u64 unrolled__226;
  // The data plane's state header, as it follows the cpu header on every packet.
  u32 st_s32_0;
  u32 st_s32_1;
  u32 st_s32_2;
  u32 st_s32_3;
  u32 st_s32_4;
  u32 st_s32_5;
  u32 st_s32_6;
  u32 st_s32_7;
  u32 st_s32_8;

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

  now = ((time_ns_t)bswap32(cpu_hdr_extra->time)) << 16;


  if (bswap16(cpu_hdr->code_path) == 1) {
    // EP node  52537
    // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  52538
    // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  52539
    // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
    u8* hdr_2 = packet_consume(pkt, 20);
    // EP node  52540
    // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
    if ((0) == (((u8)(*(u8*)(hdr_2 + 13))) & (2))) {
      // EP node  52541
      // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
      // EP node  52544
      // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
      if ((0) == (bswap32(cpu_hdr_extra->bf_query_estimate__14))) {
        // EP node  52545
        // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
        // EP node  217736
        // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
        u32 unrolled_0 = (bswap32(cpu_hdr_extra->st_s32_2)) ^ (bswap32(*(u32*)(hdr_1 + 12)));
        // EP node  220508
        // BDD node 250:op_xor(a:(ReadLSB w32 (w32 0) unrolled__15), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 272) packet_chunks) (Read w8 (w32 274) packet_chunks)) (Read w8 (w32 275) packet_chunks)))
        u32 unrolled_1 = (bswap32(cpu_hdr_extra->st_s32_3)) ^ (bswap32(*(u32*)(hdr_1 + 16)));
        // EP node  223289
        // BDD node 28:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__25) (ReadLSB w32 (w32 0) unrolled__12)), n:(w32 5))
        u32 rotated_0 = libnf::rotate_left((bswap32(cpu_hdr_extra->st_s32_7)) ^ (bswap32(cpu_hdr_extra->st_s32_1)), 5);
        // EP node  225769
        // BDD node 29:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) unrolled__15) (Concat w32 (Concat w24 (ReadMSB w16 (w32 272) packet_chunks) (Read w8 (w32 274) packet_chunks)) (Read w8 (w32 275) packet_chunks))), n:(w32 8))
        u32 rotated_1 = libnf::rotate_left((bswap32(cpu_hdr_extra->st_s32_3)) ^ (bswap32(*(u32*)(hdr_1 + 16))), 8);
        // EP node  227946
        // BDD node 30:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__16) (ReadLSB w32 (w32 0) unrolled__17)), n:(w32 16))
        u32 rotated_2 = libnf::rotate_left((unrolled_0) + (bswap32(cpu_hdr_extra->st_s32_4)), 16);
        // EP node  229818
        // BDD node 249:op_add(a:(ReadLSB w32 (w32 0) unrolled__16), b:(ReadLSB w32 (w32 0) unrolled__17))
        u32 unrolled_2 = (unrolled_0) + (bswap32(cpu_hdr_extra->st_s32_4));
        // EP node  232009
        // BDD node 31:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__28) (ReadLSB w32 (w32 0) unrolled__18)), n:(w32 13))
        u32 rotated_3 = libnf::rotate_left((rotated_0) ^ (unrolled_2), 13);
        // EP node  233893
        // BDD node 251:op_add(a:(ReadLSB w32 (w32 0) rotated__27), b:(ReadLSB w32 (w32 0) unrolled__19))
        u32 unrolled_3 = (bswap32(cpu_hdr_extra->st_s32_6)) + (unrolled_1);
        // EP node  236098
        // BDD node 32:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__29) (ReadLSB w32 (w32 0) unrolled__20)), n:(w32 7))
        u32 rotated_4 = libnf::rotate_left((rotated_1) ^ (unrolled_3), 7);
        // EP node  237994
        // BDD node 252:op_xor(a:(ReadLSB w32 (w32 0) rotated__28), b:(ReadLSB w32 (w32 0) unrolled__18))
        u32 unrolled_4 = (rotated_0) ^ (unrolled_2);
        // EP node  240213
        // BDD node 33:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__20) (ReadLSB w32 (w32 0) unrolled__21)), n:(w32 16))
        u32 rotated_5 = libnf::rotate_left((unrolled_3) + (unrolled_4), 16);
        // EP node  242121
        // BDD node 253:op_add(a:(ReadLSB w32 (w32 0) unrolled__20), b:(ReadLSB w32 (w32 0) unrolled__21))
        u32 unrolled_5 = (unrolled_3) + (unrolled_4);
        // EP node  244354
        // BDD node 254:op_xor(a:(ReadLSB w32 (w32 0) rotated__29), b:(ReadLSB w32 (w32 0) unrolled__20))
        u32 unrolled_6 = (rotated_1) ^ (unrolled_3);
        // EP node  246594
        // BDD node 255:op_add(a:(ReadLSB w32 (w32 0) rotated__30), b:(ReadLSB w32 (w32 0) unrolled__23))
        u32 unrolled_7 = (rotated_2) + (unrolled_6);
        // EP node  249162
        // BDD node 256:op_xor(a:(ReadLSB w32 (w32 0) rotated__31), b:(ReadLSB w32 (w32 0) unrolled__22))
        u32 unrolled_8 = (rotated_3) ^ (unrolled_5);
        // EP node  252060
        // BDD node 258:op_xor(a:(ReadLSB w32 (w32 0) rotated__32), b:(ReadLSB w32 (w32 0) unrolled__24))
        u32 unrolled_9 = (rotated_4) ^ (unrolled_7);
        // EP node  254967
        // BDD node 34:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__31) (ReadLSB w32 (w32 0) unrolled__22)), n:(w32 5))
        u32 rotated_6 = libnf::rotate_left((rotated_3) ^ (unrolled_5), 5);
        // EP node  257559
        // BDD node 35:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__32) (ReadLSB w32 (w32 0) unrolled__24)), n:(w32 8))
        u32 rotated_7 = libnf::rotate_left((rotated_4) ^ (unrolled_7), 8);
        // EP node  259834
        // BDD node 36:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__24) (ReadLSB w32 (w32 0) unrolled__25)), n:(w32 16))
        u32 rotated_8 = libnf::rotate_left((unrolled_7) + (unrolled_8), 16);
        // EP node  261790
        // BDD node 257:op_add(a:(ReadLSB w32 (w32 0) unrolled__24), b:(ReadLSB w32 (w32 0) unrolled__25))
        u32 unrolled_10 = (unrolled_7) + (unrolled_8);
        // EP node  264079
        // BDD node 37:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__34) (ReadLSB w32 (w32 0) unrolled__26)), n:(w32 13))
        u32 rotated_9 = libnf::rotate_left((rotated_6) ^ (unrolled_10), 13);
        // EP node  266047
        // BDD node 259:op_add(a:(ReadLSB w32 (w32 0) rotated__33), b:(ReadLSB w32 (w32 0) unrolled__27))
        u32 unrolled_11 = (rotated_5) + (unrolled_9);
        // EP node  268350
        // BDD node 38:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__35) (ReadLSB w32 (w32 0) unrolled__28)), n:(w32 7))
        u32 rotated_10 = libnf::rotate_left((rotated_7) ^ (unrolled_11), 7);
        // EP node  270330
        // BDD node 260:op_xor(a:(ReadLSB w32 (w32 0) rotated__34), b:(ReadLSB w32 (w32 0) unrolled__26))
        u32 unrolled_12 = (rotated_6) ^ (unrolled_10);
        // EP node  272647
        // BDD node 39:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__28) (ReadLSB w32 (w32 0) unrolled__29)), n:(w32 16))
        u32 rotated_11 = libnf::rotate_left((unrolled_11) + (unrolled_12), 16);
        // EP node  274639
        // BDD node 261:op_add(a:(ReadLSB w32 (w32 0) unrolled__28), b:(ReadLSB w32 (w32 0) unrolled__29))
        u32 unrolled_13 = (unrolled_11) + (unrolled_12);
        // EP node  276970
        // BDD node 262:op_xor(a:(ReadLSB w32 (w32 0) rotated__35), b:(ReadLSB w32 (w32 0) unrolled__28))
        u32 unrolled_14 = (rotated_7) ^ (unrolled_11);
        // EP node  279308
        // BDD node 263:op_add(a:(ReadLSB w32 (w32 0) rotated__36), b:(ReadLSB w32 (w32 0) unrolled__31))
        u32 unrolled_15 = (rotated_8) + (unrolled_14);
        // EP node  281988
        // BDD node 264:op_xor(a:(ReadLSB w32 (w32 0) rotated__38), b:(ReadLSB w32 (w32 0) unrolled__32))
        u32 unrolled_16 = (rotated_10) ^ (unrolled_15);
        // EP node  284340
        // BDD node 268:op_xor(a:(ReadLSB w32 (w32 0) rotated__37), b:(ReadLSB w32 (w32 0) unrolled__30))
        u32 unrolled_17 = (rotated_9) ^ (unrolled_13);
        // EP node  286362
        // BDD node 265:op_shl(a:(ZExt w32 (ReadMSB w16 (w32 512) packet_chunks)), b:(w32 16))
        u32 unrolled_18 = ((u16)(bswap16(*(u16*)(hdr_2 + 0)))) << (16);
        // EP node  288390
        // BDD node 266:op_or(a:(ReadLSB w32 (w32 0) unrolled__34), b:(ZExt w32 (ReadMSB w16 (w32 514) packet_chunks)))
        u32 unrolled_19 = (unrolled_18) | ((u16)(bswap16(*(u16*)(hdr_2 + 2))));
        // EP node  290763
        // BDD node 267:op_xor(a:(ReadLSB w32 (w32 0) unrolled__32), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 272) packet_chunks) (Read w8 (w32 274) packet_chunks)) (Read w8 (w32 275) packet_chunks)))
        u32 unrolled_20 = (unrolled_15) ^ (bswap32(*(u32*)(hdr_1 + 16)));
        // EP node  293483
        // BDD node 270:op_xor(a:(ReadLSB w32 (w32 0) unrolled__33), b:(ReadLSB w32 (w32 0) unrolled__35))
        u32 unrolled_21 = (unrolled_16) ^ (unrolled_19);
        // EP node  296211
        // BDD node 40:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__37) (ReadLSB w32 (w32 0) unrolled__30)), n:(w32 5))
        u32 rotated_12 = libnf::rotate_left((rotated_9) ^ (unrolled_13), 5);
        // EP node  298605
        // BDD node 41:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) unrolled__33) (ReadLSB w32 (w32 0) unrolled__35)), n:(w32 8))
        u32 rotated_13 = libnf::rotate_left((unrolled_16) ^ (unrolled_19), 8);
        // EP node  300663
        // BDD node 42:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__36) (ReadLSB w32 (w32 0) unrolled__37)), n:(w32 16))
        u32 rotated_14 = libnf::rotate_left((unrolled_20) + (unrolled_17), 16);
        // EP node  302383
        // BDD node 269:op_add(a:(ReadLSB w32 (w32 0) unrolled__36), b:(ReadLSB w32 (w32 0) unrolled__37))
        u32 unrolled_22 = (unrolled_20) + (unrolled_17);
        // EP node  304453
        // BDD node 43:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__40) (ReadLSB w32 (w32 0) unrolled__38)), n:(w32 13))
        u32 rotated_15 = libnf::rotate_left((rotated_12) ^ (unrolled_22), 13);
        // EP node  306183
        // BDD node 271:op_add(a:(ReadLSB w32 (w32 0) rotated__39), b:(ReadLSB w32 (w32 0) unrolled__39))
        u32 unrolled_23 = (rotated_11) + (unrolled_21);
        // EP node  308265
        // BDD node 44:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__41) (ReadLSB w32 (w32 0) unrolled__40)), n:(w32 7))
        u32 rotated_16 = libnf::rotate_left((rotated_13) ^ (unrolled_23), 7);
        // EP node  310005
        // BDD node 272:op_xor(a:(ReadLSB w32 (w32 0) rotated__40), b:(ReadLSB w32 (w32 0) unrolled__38))
        u32 unrolled_24 = (rotated_12) ^ (unrolled_22);
        // EP node  312099
        // BDD node 45:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__40) (ReadLSB w32 (w32 0) unrolled__41)), n:(w32 16))
        u32 rotated_17 = libnf::rotate_left((unrolled_23) + (unrolled_24), 16);
        // EP node  313849
        // BDD node 273:op_add(a:(ReadLSB w32 (w32 0) unrolled__40), b:(ReadLSB w32 (w32 0) unrolled__41))
        u32 unrolled_25 = (unrolled_23) + (unrolled_24);
        // EP node  315955
        // BDD node 274:op_xor(a:(ReadLSB w32 (w32 0) rotated__41), b:(ReadLSB w32 (w32 0) unrolled__40))
        u32 unrolled_26 = (rotated_13) ^ (unrolled_23);
        // EP node  318067
        // BDD node 275:op_add(a:(ReadLSB w32 (w32 0) rotated__42), b:(ReadLSB w32 (w32 0) unrolled__43))
        u32 unrolled_27 = (rotated_14) + (unrolled_26);
        // EP node  320538
        // BDD node 276:op_xor(a:(ReadLSB w32 (w32 0) rotated__43), b:(ReadLSB w32 (w32 0) unrolled__42))
        u32 unrolled_28 = (rotated_15) ^ (unrolled_25);
        // EP node  323370
        // BDD node 278:op_xor(a:(ReadLSB w32 (w32 0) rotated__44), b:(ReadLSB w32 (w32 0) unrolled__44))
        u32 unrolled_29 = (rotated_16) ^ (unrolled_27);
        // EP node  326210
        // BDD node 46:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__43) (ReadLSB w32 (w32 0) unrolled__42)), n:(w32 5))
        u32 rotated_18 = libnf::rotate_left((rotated_15) ^ (unrolled_25), 5);
        // EP node  328702
        // BDD node 47:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__44) (ReadLSB w32 (w32 0) unrolled__44)), n:(w32 8))
        u32 rotated_19 = libnf::rotate_left((rotated_16) ^ (unrolled_27), 8);
        // EP node  330844
        // BDD node 48:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__44) (ReadLSB w32 (w32 0) unrolled__45)), n:(w32 16))
        u32 rotated_20 = libnf::rotate_left((unrolled_27) + (unrolled_28), 16);
        // EP node  332634
        // BDD node 277:op_add(a:(ReadLSB w32 (w32 0) unrolled__44), b:(ReadLSB w32 (w32 0) unrolled__45))
        u32 unrolled_30 = (unrolled_27) + (unrolled_28);
        // EP node  334788
        // BDD node 49:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__46) (ReadLSB w32 (w32 0) unrolled__46)), n:(w32 13))
        u32 rotated_21 = libnf::rotate_left((rotated_18) ^ (unrolled_30), 13);
        // EP node  336588
        // BDD node 279:op_add(a:(ReadLSB w32 (w32 0) rotated__45), b:(ReadLSB w32 (w32 0) unrolled__47))
        u32 unrolled_31 = (rotated_17) + (unrolled_29);
        // EP node  338754
        // BDD node 50:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__47) (ReadLSB w32 (w32 0) unrolled__48)), n:(w32 7))
        u32 rotated_22 = libnf::rotate_left((rotated_19) ^ (unrolled_31), 7);
        // EP node  340564
        // BDD node 280:op_xor(a:(ReadLSB w32 (w32 0) rotated__46), b:(ReadLSB w32 (w32 0) unrolled__46))
        u32 unrolled_32 = (rotated_18) ^ (unrolled_30);
        // EP node  342742
        // BDD node 51:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__48) (ReadLSB w32 (w32 0) unrolled__49)), n:(w32 16))
        u32 rotated_23 = libnf::rotate_left((unrolled_31) + (unrolled_32), 16);
        // EP node  344562
        // BDD node 281:op_add(a:(ReadLSB w32 (w32 0) unrolled__48), b:(ReadLSB w32 (w32 0) unrolled__49))
        u32 unrolled_33 = (unrolled_31) + (unrolled_32);
        // EP node  346752
        // BDD node 282:op_xor(a:(ReadLSB w32 (w32 0) rotated__47), b:(ReadLSB w32 (w32 0) unrolled__48))
        u32 unrolled_34 = (rotated_19) ^ (unrolled_31);
        // EP node  348948
        // BDD node 283:op_add(a:(ReadLSB w32 (w32 0) rotated__48), b:(ReadLSB w32 (w32 0) unrolled__51))
        u32 unrolled_35 = (rotated_20) + (unrolled_34);
        // EP node  351517
        // BDD node 284:op_xor(a:(ReadLSB w32 (w32 0) rotated__50), b:(ReadLSB w32 (w32 0) unrolled__52))
        u32 unrolled_36 = (rotated_22) ^ (unrolled_35);
        // EP node  353725
        // BDD node 287:op_xor(a:(ReadLSB w32 (w32 0) rotated__49), b:(ReadLSB w32 (w32 0) unrolled__50))
        u32 unrolled_37 = (rotated_21) ^ (unrolled_33);
        // EP node  355570
        // BDD node 285:op_add(a:(w32 4294967295), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))
        u32 unrolled_38 = (4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4)));
        // EP node  357790
        // BDD node 286:op_xor(a:(ReadLSB w32 (w32 0) unrolled__52), b:(ReadLSB w32 (w32 0) unrolled__35))
        u32 unrolled_39 = (unrolled_35) ^ (unrolled_19);
        // EP node  360387
        // BDD node 289:op_xor(a:(ReadLSB w32 (w32 0) unrolled__53), b:(ReadLSB w32 (w32 0) unrolled__54))
        u32 unrolled_40 = (unrolled_36) ^ (unrolled_38);
        // EP node  362991
        // BDD node 52:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__49) (ReadLSB w32 (w32 0) unrolled__50)), n:(w32 5))
        u32 rotated_24 = libnf::rotate_left((rotated_21) ^ (unrolled_33), 5);
        // EP node  365229
        // BDD node 53:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) unrolled__53) (ReadLSB w32 (w32 0) unrolled__54)), n:(w32 8))
        u32 rotated_25 = libnf::rotate_left((unrolled_36) ^ (unrolled_38), 8);
        // EP node  367099
        // BDD node 54:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__55) (ReadLSB w32 (w32 0) unrolled__56)), n:(w32 16))
        u32 rotated_26 = libnf::rotate_left((unrolled_39) + (unrolled_37), 16);
        // EP node  368599
        // BDD node 288:op_add(a:(ReadLSB w32 (w32 0) unrolled__55), b:(ReadLSB w32 (w32 0) unrolled__56))
        u32 unrolled_41 = (unrolled_39) + (unrolled_37);
        // EP node  370479
        // BDD node 55:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__52) (ReadLSB w32 (w32 0) unrolled__57)), n:(w32 13))
        u32 rotated_27 = libnf::rotate_left((rotated_24) ^ (unrolled_41), 13);
        // EP node  371987
        // BDD node 290:op_add(a:(ReadLSB w32 (w32 0) rotated__51), b:(ReadLSB w32 (w32 0) unrolled__58))
        u32 unrolled_42 = (rotated_23) + (unrolled_40);
        // EP node  373877
        // BDD node 56:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__53) (ReadLSB w32 (w32 0) unrolled__59)), n:(w32 7))
        u32 rotated_28 = libnf::rotate_left((rotated_25) ^ (unrolled_42), 7);
        // EP node  375393
        // BDD node 291:op_xor(a:(ReadLSB w32 (w32 0) rotated__52), b:(ReadLSB w32 (w32 0) unrolled__57))
        u32 unrolled_43 = (rotated_24) ^ (unrolled_41);
        // EP node  377293
        // BDD node 57:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__59) (ReadLSB w32 (w32 0) unrolled__60)), n:(w32 16))
        u32 rotated_29 = libnf::rotate_left((unrolled_42) + (unrolled_43), 16);
        // EP node  378817
        // BDD node 292:op_add(a:(ReadLSB w32 (w32 0) unrolled__59), b:(ReadLSB w32 (w32 0) unrolled__60))
        u32 unrolled_44 = (unrolled_42) + (unrolled_43);
        // EP node  380727
        // BDD node 293:op_xor(a:(ReadLSB w32 (w32 0) rotated__53), b:(ReadLSB w32 (w32 0) unrolled__59))
        u32 unrolled_45 = (rotated_25) ^ (unrolled_42);
        // EP node  382642
        // BDD node 294:op_add(a:(ReadLSB w32 (w32 0) rotated__54), b:(ReadLSB w32 (w32 0) unrolled__62))
        u32 unrolled_46 = (rotated_26) + (unrolled_45);
        // EP node  384946
        // BDD node 295:op_xor(a:(ReadLSB w32 (w32 0) rotated__55), b:(ReadLSB w32 (w32 0) unrolled__61))
        u32 unrolled_47 = (rotated_27) ^ (unrolled_44);
        // EP node  387641
        // BDD node 297:op_xor(a:(ReadLSB w32 (w32 0) rotated__56), b:(ReadLSB w32 (w32 0) unrolled__63))
        u32 unrolled_48 = (rotated_28) ^ (unrolled_46);
        // EP node  390343
        // BDD node 58:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__55) (ReadLSB w32 (w32 0) unrolled__61)), n:(w32 5))
        u32 rotated_30 = libnf::rotate_left((rotated_27) ^ (unrolled_44), 5);
        // EP node  392665
        // BDD node 59:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__56) (ReadLSB w32 (w32 0) unrolled__63)), n:(w32 8))
        u32 rotated_31 = libnf::rotate_left((rotated_28) ^ (unrolled_46), 8);
        // EP node  394605
        // BDD node 60:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__63) (ReadLSB w32 (w32 0) unrolled__64)), n:(w32 16))
        u32 rotated_32 = libnf::rotate_left((unrolled_46) + (unrolled_47), 16);
        // EP node  396161
        // BDD node 296:op_add(a:(ReadLSB w32 (w32 0) unrolled__63), b:(ReadLSB w32 (w32 0) unrolled__64))
        u32 unrolled_49 = (unrolled_46) + (unrolled_47);
        // EP node  398111
        // BDD node 61:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__58) (ReadLSB w32 (w32 0) unrolled__65)), n:(w32 13))
        u32 rotated_33 = libnf::rotate_left((rotated_30) ^ (unrolled_49), 13);
        // EP node  399675
        // BDD node 298:op_add(a:(ReadLSB w32 (w32 0) rotated__57), b:(ReadLSB w32 (w32 0) unrolled__66))
        u32 unrolled_50 = (rotated_29) + (unrolled_48);
        // EP node  401635
        // BDD node 62:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__59) (ReadLSB w32 (w32 0) unrolled__67)), n:(w32 7))
        u32 rotated_34 = libnf::rotate_left((rotated_31) ^ (unrolled_50), 7);
        // EP node  403207
        // BDD node 299:op_xor(a:(ReadLSB w32 (w32 0) rotated__58), b:(ReadLSB w32 (w32 0) unrolled__65))
        u32 unrolled_51 = (rotated_30) ^ (unrolled_49);
        // EP node  405177
        // BDD node 63:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__67) (ReadLSB w32 (w32 0) unrolled__68)), n:(w32 16))
        u32 rotated_35 = libnf::rotate_left((unrolled_50) + (unrolled_51), 16);
        // EP node  406757
        // BDD node 300:op_add(a:(ReadLSB w32 (w32 0) unrolled__67), b:(ReadLSB w32 (w32 0) unrolled__68))
        u32 unrolled_52 = (unrolled_50) + (unrolled_51);
        // EP node  408737
        // BDD node 301:op_xor(a:(ReadLSB w32 (w32 0) rotated__59), b:(ReadLSB w32 (w32 0) unrolled__67))
        u32 unrolled_53 = (rotated_31) ^ (unrolled_50);
        // EP node  410722
        // BDD node 302:op_add(a:(ReadLSB w32 (w32 0) rotated__60), b:(ReadLSB w32 (w32 0) unrolled__70))
        u32 unrolled_54 = (rotated_32) + (unrolled_53);
        // EP node  413508
        // BDD node 304:op_xor(a:(ReadLSB w32 (w32 0) rotated__61), b:(ReadLSB w32 (w32 0) unrolled__69))
        u32 unrolled_55 = (rotated_33) ^ (unrolled_52);
        // EP node  415902
        // BDD node 306:op_xor(a:(ReadLSB w32 (w32 0) rotated__62), b:(ReadLSB w32 (w32 0) unrolled__71))
        u32 unrolled_56 = (rotated_34) ^ (unrolled_54);
        // EP node  418302
        // BDD node 303:op_xor(a:(ReadLSB w32 (w32 0) unrolled__71), b:(ReadLSB w32 (w32 0) unrolled__54))
        u32 unrolled_57 = (unrolled_54) ^ (unrolled_38);
        // EP node  421109
        // BDD node 64:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__61) (ReadLSB w32 (w32 0) unrolled__69)), n:(w32 5))
        u32 rotated_36 = libnf::rotate_left((rotated_33) ^ (unrolled_52), 5);
        // EP node  423521
        // BDD node 65:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__62) (ReadLSB w32 (w32 0) unrolled__71)), n:(w32 8))
        u32 rotated_37 = libnf::rotate_left((rotated_34) ^ (unrolled_54), 8);
        // EP node  425536
        // BDD node 66:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__72) (ReadLSB w32 (w32 0) unrolled__73)), n:(w32 16))
        u32 rotated_38 = libnf::rotate_left((unrolled_57) + (unrolled_55), 16);
        // EP node  427152
        // BDD node 305:op_add(a:(ReadLSB w32 (w32 0) unrolled__72), b:(ReadLSB w32 (w32 0) unrolled__73))
        u32 unrolled_58 = (unrolled_57) + (unrolled_55);
        // EP node  429177
        // BDD node 67:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__64) (ReadLSB w32 (w32 0) unrolled__74)), n:(w32 13))
        u32 rotated_39 = libnf::rotate_left((rotated_36) ^ (unrolled_58), 13);
        // EP node  430801
        // BDD node 307:op_add(a:(ReadLSB w32 (w32 0) rotated__63), b:(ReadLSB w32 (w32 0) unrolled__75))
        u32 unrolled_59 = (rotated_35) + (unrolled_56);
        // EP node  432836
        // BDD node 68:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__65) (ReadLSB w32 (w32 0) unrolled__76)), n:(w32 7))
        u32 rotated_40 = libnf::rotate_left((rotated_37) ^ (unrolled_59), 7);
        // EP node  434468
        // BDD node 308:op_xor(a:(ReadLSB w32 (w32 0) rotated__64), b:(ReadLSB w32 (w32 0) unrolled__74))
        u32 unrolled_60 = (rotated_36) ^ (unrolled_58);
        // EP node  436513
        // BDD node 69:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__76) (ReadLSB w32 (w32 0) unrolled__77)), n:(w32 16))
        u32 rotated_41 = libnf::rotate_left((unrolled_59) + (unrolled_60), 16);
        // EP node  438153
        // BDD node 309:op_add(a:(ReadLSB w32 (w32 0) unrolled__76), b:(ReadLSB w32 (w32 0) unrolled__77))
        u32 unrolled_61 = (unrolled_59) + (unrolled_60);
        // EP node  440208
        // BDD node 310:op_xor(a:(ReadLSB w32 (w32 0) rotated__65), b:(ReadLSB w32 (w32 0) unrolled__76))
        u32 unrolled_62 = (rotated_37) ^ (unrolled_59);
        // EP node  442268
        // BDD node 311:op_add(a:(ReadLSB w32 (w32 0) rotated__66), b:(ReadLSB w32 (w32 0) unrolled__79))
        u32 unrolled_63 = (rotated_38) + (unrolled_62);
        // EP node  444746
        // BDD node 312:op_xor(a:(ReadLSB w32 (w32 0) rotated__67), b:(ReadLSB w32 (w32 0) unrolled__78))
        u32 unrolled_64 = (rotated_39) ^ (unrolled_61);
        // EP node  447644
        // BDD node 314:op_xor(a:(ReadLSB w32 (w32 0) rotated__68), b:(ReadLSB w32 (w32 0) unrolled__80))
        u32 unrolled_65 = (rotated_40) ^ (unrolled_63);
        // EP node  450549
        // BDD node 70:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__67) (ReadLSB w32 (w32 0) unrolled__78)), n:(w32 5))
        u32 rotated_42 = libnf::rotate_left((rotated_39) ^ (unrolled_61), 5);
        // EP node  453045
        // BDD node 71:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__68) (ReadLSB w32 (w32 0) unrolled__80)), n:(w32 8))
        u32 rotated_43 = libnf::rotate_left((rotated_40) ^ (unrolled_63), 8);
        // EP node  455130
        // BDD node 72:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__80) (ReadLSB w32 (w32 0) unrolled__81)), n:(w32 16))
        u32 rotated_44 = libnf::rotate_left((unrolled_63) + (unrolled_64), 16);
        // EP node  456802
        // BDD node 313:op_add(a:(ReadLSB w32 (w32 0) unrolled__80), b:(ReadLSB w32 (w32 0) unrolled__81))
        u32 unrolled_66 = (unrolled_63) + (unrolled_64);
        // EP node  458897
        // BDD node 73:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__70) (ReadLSB w32 (w32 0) unrolled__82)), n:(w32 13))
        u32 rotated_45 = libnf::rotate_left((rotated_42) ^ (unrolled_66), 13);
        // EP node  460577
        // BDD node 315:op_add(a:(ReadLSB w32 (w32 0) rotated__69), b:(ReadLSB w32 (w32 0) unrolled__83))
        u32 unrolled_67 = (rotated_41) + (unrolled_65);
        // EP node  462682
        // BDD node 74:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__71) (ReadLSB w32 (w32 0) unrolled__84)), n:(w32 7))
        u32 rotated_46 = libnf::rotate_left((rotated_43) ^ (unrolled_67), 7);
        // EP node  464370
        // BDD node 316:op_xor(a:(ReadLSB w32 (w32 0) rotated__70), b:(ReadLSB w32 (w32 0) unrolled__82))
        u32 unrolled_68 = (rotated_42) ^ (unrolled_66);
        // EP node  466485
        // BDD node 75:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__84) (ReadLSB w32 (w32 0) unrolled__85)), n:(w32 16))
        u32 rotated_47 = libnf::rotate_left((unrolled_67) + (unrolled_68), 16);
        // EP node  468181
        // BDD node 317:op_add(a:(ReadLSB w32 (w32 0) unrolled__84), b:(ReadLSB w32 (w32 0) unrolled__85))
        u32 unrolled_69 = (unrolled_67) + (unrolled_68);
        // EP node  470306
        // BDD node 318:op_xor(a:(ReadLSB w32 (w32 0) rotated__71), b:(ReadLSB w32 (w32 0) unrolled__84))
        u32 unrolled_70 = (rotated_43) ^ (unrolled_67);
        // EP node  472436
        // BDD node 319:op_add(a:(ReadLSB w32 (w32 0) rotated__72), b:(ReadLSB w32 (w32 0) unrolled__87))
        u32 unrolled_71 = (rotated_44) + (unrolled_70);
        // EP node  474998
        // BDD node 320:op_xor(a:(ReadLSB w32 (w32 0) rotated__73), b:(ReadLSB w32 (w32 0) unrolled__86))
        u32 unrolled_72 = (rotated_45) ^ (unrolled_69);
        // EP node  477994
        // BDD node 322:op_xor(a:(ReadLSB w32 (w32 0) rotated__74), b:(ReadLSB w32 (w32 0) unrolled__88))
        u32 unrolled_73 = (rotated_46) ^ (unrolled_71);
        // EP node  480997
        // BDD node 76:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__73) (ReadLSB w32 (w32 0) unrolled__86)), n:(w32 5))
        u32 rotated_48 = libnf::rotate_left((rotated_45) ^ (unrolled_69), 5);
        // EP node  483577
        // BDD node 77:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__74) (ReadLSB w32 (w32 0) unrolled__88)), n:(w32 8))
        u32 rotated_49 = libnf::rotate_left((rotated_46) ^ (unrolled_71), 8);
        // EP node  485732
        // BDD node 78:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__88) (ReadLSB w32 (w32 0) unrolled__89)), n:(w32 16))
        u32 rotated_50 = libnf::rotate_left((unrolled_71) + (unrolled_72), 16);
        // EP node  487460
        // BDD node 321:op_add(a:(ReadLSB w32 (w32 0) unrolled__88), b:(ReadLSB w32 (w32 0) unrolled__89))
        u32 unrolled_74 = (unrolled_71) + (unrolled_72);
        // EP node  489625
        // BDD node 79:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__76) (ReadLSB w32 (w32 0) unrolled__90)), n:(w32 13))
        u32 rotated_51 = libnf::rotate_left((rotated_48) ^ (unrolled_74), 13);
        // EP node  491361
        // BDD node 323:op_add(a:(ReadLSB w32 (w32 0) rotated__75), b:(ReadLSB w32 (w32 0) unrolled__91))
        u32 unrolled_75 = (rotated_47) + (unrolled_73);
        // EP node  493536
        // BDD node 80:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__77) (ReadLSB w32 (w32 0) unrolled__92)), n:(w32 7))
        u32 rotated_52 = libnf::rotate_left((rotated_49) ^ (unrolled_75), 7);
        // EP node  495280
        // BDD node 324:op_xor(a:(ReadLSB w32 (w32 0) rotated__76), b:(ReadLSB w32 (w32 0) unrolled__90))
        u32 unrolled_76 = (rotated_48) ^ (unrolled_74);
        // EP node  497465
        // BDD node 81:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__92) (ReadLSB w32 (w32 0) unrolled__93)), n:(w32 16))
        u32 rotated_53 = libnf::rotate_left((unrolled_75) + (unrolled_76), 16);
        // EP node  499217
        // BDD node 325:op_add(a:(ReadLSB w32 (w32 0) unrolled__92), b:(ReadLSB w32 (w32 0) unrolled__93))
        u32 unrolled_77 = (unrolled_75) + (unrolled_76);
        // EP node  501412
        // BDD node 326:op_xor(a:(ReadLSB w32 (w32 0) rotated__77), b:(ReadLSB w32 (w32 0) unrolled__92))
        u32 unrolled_78 = (rotated_49) ^ (unrolled_75);
        // EP node  503612
        // BDD node 327:op_add(a:(ReadLSB w32 (w32 0) rotated__78), b:(ReadLSB w32 (w32 0) unrolled__95))
        u32 unrolled_79 = (rotated_50) + (unrolled_78);
        // EP node  506258
        // BDD node 328:op_xor(a:(ReadLSB w32 (w32 0) rotated__79), b:(ReadLSB w32 (w32 0) unrolled__94))
        u32 unrolled_80 = (rotated_51) ^ (unrolled_77);
        // EP node  509352
        // BDD node 330:op_xor(a:(ReadLSB w32 (w32 0) rotated__80), b:(ReadLSB w32 (w32 0) unrolled__96))
        u32 unrolled_81 = (rotated_52) ^ (unrolled_79);
        // EP node  512453
        // BDD node 82:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__79) (ReadLSB w32 (w32 0) unrolled__94)), n:(w32 5))
        u32 rotated_54 = libnf::rotate_left((rotated_51) ^ (unrolled_77), 5);
        // EP node  515117
        // BDD node 83:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__80) (ReadLSB w32 (w32 0) unrolled__96)), n:(w32 8))
        u32 rotated_55 = libnf::rotate_left((rotated_52) ^ (unrolled_79), 8);
        // EP node  517342
        // BDD node 84:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__96) (ReadLSB w32 (w32 0) unrolled__97)), n:(w32 16))
        u32 rotated_56 = libnf::rotate_left((unrolled_79) + (unrolled_80), 16);
        // EP node  519126
        // BDD node 329:op_add(a:(ReadLSB w32 (w32 0) unrolled__96), b:(ReadLSB w32 (w32 0) unrolled__97))
        u32 unrolled_82 = (unrolled_79) + (unrolled_80);
        // EP node  521361
        // BDD node 85:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__82) (ReadLSB w32 (w32 0) unrolled__98)), n:(w32 13))
        u32 rotated_57 = libnf::rotate_left((rotated_54) ^ (unrolled_82), 13);
        // EP node  523153
        // BDD node 331:op_add(a:(ReadLSB w32 (w32 0) rotated__81), b:(ReadLSB w32 (w32 0) unrolled__99))
        u32 unrolled_83 = (rotated_53) + (unrolled_81);
        // EP node  525398
        // BDD node 86:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__83) (ReadLSB w32 (w32 0) unrolled__100)), n:(w32 7))
        u32 rotated_58 = libnf::rotate_left((rotated_55) ^ (unrolled_83), 7);
        // EP node  527198
        // BDD node 332:op_xor(a:(ReadLSB w32 (w32 0) rotated__82), b:(ReadLSB w32 (w32 0) unrolled__98))
        u32 unrolled_84 = (rotated_54) ^ (unrolled_82);
        // EP node  529453
        // BDD node 87:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__100) (ReadLSB w32 (w32 0) unrolled__101)), n:(w32 16))
        u32 rotated_59 = libnf::rotate_left((unrolled_83) + (unrolled_84), 16);
        // EP node  531261
        // BDD node 88:vector_borrow(vector:(w64 1073939616), index:(w32 0), val_out:(w64 1074050352)[ -> (w64 1073953512)])
        buffer_t value_0;
        state->vector_register_1073939616.get(0, value_0);
        // EP node  533526
        // BDD node 89:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(ReadLSB w32 (w32 0) vector_data__88)])
        // EP node  535342
        // BDD node 333:op_sub(a:(ReadLSB w32 (w32 2) next_time), b:(ReadLSB w32 (w32 0) vector_data__88))
        u32 unrolled_85 = ((now>>16) & 0xffffffffull) - ((u32)value_0.get(0, 4));
        // EP node  537162
        // BDD node 334:op_lshr(a:(ReadLSB w32 (w32 0) unrolled__102), b:(w32 12))
        u32 unrolled_86 = (unrolled_85) >> (12);
        // EP node  538530
        // BDD node 335:op_add(a:(w32 4294967295), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 520) packet_chunks) (Read w8 (w32 522) packet_chunks)) (Read w8 (w32 523) packet_chunks)))
        u32 unrolled_87 = (4294967295LL) + (bswap32(*(u32*)(hdr_2 + 8)));
        // EP node  539444
        // BDD node 336:op_xor(a:(ReadLSB w32 (w32 0) rotated__83), b:(ReadLSB w32 (w32 0) unrolled__100))
        u32 unrolled_88 = (rotated_55) ^ (unrolled_83);
        // EP node  540360
        // BDD node 337:op_add(a:(ReadLSB w32 (w32 0) rotated__84), b:(ReadLSB w32 (w32 0) unrolled__105))
        u32 unrolled_89 = (rotated_56) + (unrolled_88);
        // EP node  541278
        // BDD node 338:op_add(a:(ReadLSB w32 (w32 0) unrolled__100), b:(ReadLSB w32 (w32 0) unrolled__101))
        u32 unrolled_90 = (unrolled_83) + (unrolled_84);
        // EP node  542198
        // BDD node 339:op_xor(a:(ReadLSB w32 (w32 0) rotated__85), b:(ReadLSB w32 (w32 0) unrolled__107))
        u32 unrolled_91 = (rotated_57) ^ (unrolled_90);
        // EP node  543120
        // BDD node 340:op_xor(a:(ReadLSB w32 (w32 0) unrolled__106), b:(ReadLSB w32 (w32 0) unrolled__108))
        u32 unrolled_92 = (unrolled_89) ^ (unrolled_91);
        // EP node  544044
        // BDD node 341:op_xor(a:(ReadLSB w32 (w32 0) unrolled__109), b:(ReadLSB w32 (w32 0) rotated__87))
        u32 unrolled_93 = (unrolled_92) ^ (rotated_59);
        // EP node  544507
        // BDD node 342:op_xor(a:(ReadLSB w32 (w32 0) rotated__86), b:(ReadLSB w32 (w32 0) unrolled__106))
        u32 unrolled_94 = (rotated_58) ^ (unrolled_89);
        // EP node  544971
        // BDD node 343:op_xor(a:(ReadLSB w32 (w32 0) unrolled__110), b:(ReadLSB w32 (w32 0) unrolled__111))
        u32 unrolled_95 = (unrolled_93) ^ (unrolled_94);
        // EP node  545436
        // BDD node 344:op_xor(a:(ReadLSB w32 (w32 0) unrolled__104), b:(ReadLSB w32 (w32 0) unrolled__112))
        u32 unrolled_96 = (unrolled_87) ^ (unrolled_95);
        // EP node  545902
        // BDD node 90:if ((Ule (Sub w32 (ReadLSB w32 (w32 0) unrolled__103) (ReadLSB w32 (w32 0) unrolled__113)) (w32 2))
        if (((unrolled_86) - (unrolled_96)) <= (2)) {
          // EP node  545903
          // BDD node 90:if ((Ule (Sub w32 (ReadLSB w32 (w32 0) unrolled__103) (ReadLSB w32 (w32 0) unrolled__113)) (w32 2))
          // EP node  548718
          // BDD node 91:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073757248), l4_header:(w64 1073757504), packet:(w64 1073957496))
          trigger_update_ipv4_tcpudp_checksums = true;
          l3_hdr = (void *)hdr_1;
          l4_hdr = (void *)hdr_2;
          // EP node  549190
          // BDD node 92:packet_return_chunk(p:(w64 1074032776), the_chunk:(w64 1073757504)[(Concat w160 (Read w8 (w32 531) packet_chunks) (Concat w152 (Read w8 (w32 530) packet_chunks) (Concat w144 (Read w8 (w32 529) packet_chunks) (Concat w136 (Read w8 (w32 528) packet_chunks) (Concat w128 (Read w8 (w32 527) packet_chunks) (Concat w120 (Read w8 (w32 526) packet_chunks) (Concat w112 (Extract w8 0 (Or w32 (ZExt w32 (Read w8 (w32 525) packet_chunks)) (w32 64))) (Concat w104 (w8 80) (Concat w96 (Read w8 (w32 523) packet_chunks) (Concat w88 (Read w8 (w32 522) packet_chunks) (Concat w80 (Read w8 (w32 521) packet_chunks) (Concat w72 (Read w8 (w32 520) packet_chunks) (Concat w64 (Extract w8 0 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w56 (Extract w8 8 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w48 (Extract w8 16 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w40 (Extract w8 24 (Add w32 (w32 4294967295) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (ReadLSB w32 (w32 512) packet_chunks)))))))))))))))))])
          const u8 hdr_2_549190_b4 = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4))))>>24);
          const u8 hdr_2_549190_b5 = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4))))>>16);
          const u8 hdr_2_549190_b6 = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4))))>>8);
          const u8 hdr_2_549190_b7 = (u8)(((4294967295LL) + (bswap32(*(u32*)(hdr_2 + 4)))));
          const u8 hdr_2_549190_b12 = 80;
          const u8 hdr_2_549190_b13 = (u8)((((u8)(*(u8*)(hdr_2 + 13))) | (64)));
          hdr_2[4] = hdr_2_549190_b4;
          hdr_2[5] = hdr_2_549190_b5;
          hdr_2[6] = hdr_2_549190_b6;
          hdr_2[7] = hdr_2_549190_b7;
          hdr_2[12] = hdr_2_549190_b12;
          hdr_2[13] = hdr_2_549190_b13;
          // EP node  549663
          // BDD node 93:packet_return_chunk(p:(w64 1074032776), the_chunk:(w64 1073757248)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (Read w8 (w32 271) packet_chunks) (Concat w120 (Read w8 (w32 270) packet_chunks) (Concat w112 (Read w8 (w32 269) packet_chunks) (Concat w104 (Read w8 (w32 268) packet_chunks) (Concat w96 (Read w8 (w32 1) checksum__91) (Concat w88 (Read w8 (w32 0) checksum__91) (Concat w80 (Read w8 (w32 265) packet_chunks) (Concat w72 (Read w8 (w32 264) packet_chunks) (Concat w64 (Read w8 (w32 263) packet_chunks) (Concat w56 (Read w8 (w32 262) packet_chunks) (Concat w48 (Read w8 (w32 261) packet_chunks) (Concat w40 (Read w8 (w32 260) packet_chunks) (Concat w32 (w8 40) (Concat w24 (w8 0) (Concat w16 (Read w8 (w32 257) packet_chunks) (w8 69))))))))))))))))))))])
          const u8 hdr_1_549663_b0 = 69;
          const u8 hdr_1_549663_b2 = 0;
          const u8 hdr_1_549663_b3 = 40;
          hdr_1[0] = hdr_1_549663_b0;
          hdr_1[2] = hdr_1_549663_b2;
          hdr_1[3] = hdr_1_549663_b3;
          // EP node  550610
          // BDD node 95:FORWARD
          cpu_hdr->egress_dev = bswap16(0);
        } else {
          // EP node  545904
          // BDD node 90:if ((Ule (Sub w32 (ReadLSB w32 (w32 0) unrolled__103) (ReadLSB w32 (w32 0) unrolled__113)) (w32 2))
          // EP node  547778
          // BDD node 99:DROP
          result.forward = false;
        }
      } else {
        // EP node  52546
        // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
        // EP node  52547
        // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  52542
      // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
      // EP node  52543
      // BDD node 247:op_xor(a:(ReadLSB w32 (w32 0) unrolled__14), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    }
  }
  else if (bswap16(cpu_hdr->code_path) == 0) {
    // EP node  14034
    // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
    u8* hdr_3 = packet_consume(pkt, 14);
    // EP node  14035
    // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
    u8* hdr_4 = packet_consume(pkt, 20);
    // EP node  14036
    // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
    u8* hdr_5 = packet_consume(pkt, 20);
    // EP node  14037
    // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
    if ((0) == (((u8)(*(u8*)(hdr_5 + 13))) & (2))) {
      // EP node  14038
      // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
      // EP node  14040
      // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    } else {
      // EP node  14039
      // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
      // EP node  14041
      // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
      if ((0) == (((u8)(*(u8*)(hdr_5 + 13))) & (16))) {
        // EP node  14042
        // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
        // EP node  70223
        // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
        u32 unrolled_97 = (bswap32(cpu_hdr_extra->st_s32_2)) ^ (bswap32(*(u32*)(hdr_4 + 12)));
        // EP node  71266
        // BDD node 364:op_xor(a:(ReadLSB w32 (w32 0) unrolled__129), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 272) packet_chunks) (Read w8 (w32 274) packet_chunks)) (Read w8 (w32 275) packet_chunks)))
        u32 unrolled_98 = (bswap32(cpu_hdr_extra->st_s32_3)) ^ (bswap32(*(u32*)(hdr_4 + 16)));
        // EP node  72316
        // BDD node 119:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__116) (ReadLSB w32 (w32 0) unrolled__126)), n:(w32 5))
        u32 rotated_60 = libnf::rotate_left((bswap32(cpu_hdr_extra->st_s32_7)) ^ (bswap32(cpu_hdr_extra->st_s32_1)), 5);
        // EP node  73222
        // BDD node 120:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) unrolled__129) (Concat w32 (Concat w24 (ReadMSB w16 (w32 272) packet_chunks) (Read w8 (w32 274) packet_chunks)) (Read w8 (w32 275) packet_chunks))), n:(w32 8))
        u32 rotated_61 = libnf::rotate_left((bswap32(cpu_hdr_extra->st_s32_3)) ^ (bswap32(*(u32*)(hdr_4 + 16))), 8);
        // EP node  73982
        // BDD node 121:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__130) (ReadLSB w32 (w32 0) unrolled__131)), n:(w32 16))
        u32 rotated_62 = libnf::rotate_left((unrolled_97) + (bswap32(cpu_hdr_extra->st_s32_4)), 16);
        // EP node  74594
        // BDD node 363:op_add(a:(ReadLSB w32 (w32 0) unrolled__130), b:(ReadLSB w32 (w32 0) unrolled__131))
        u32 unrolled_99 = (unrolled_97) + (bswap32(cpu_hdr_extra->st_s32_4));
        // EP node  75364
        // BDD node 122:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__119) (ReadLSB w32 (w32 0) unrolled__132)), n:(w32 13))
        u32 rotated_63 = libnf::rotate_left((rotated_60) ^ (unrolled_99), 13);
        // EP node  75984
        // BDD node 365:op_add(a:(ReadLSB w32 (w32 0) rotated__118), b:(ReadLSB w32 (w32 0) unrolled__133))
        u32 unrolled_100 = (bswap32(cpu_hdr_extra->st_s32_6)) + (unrolled_98);
        // EP node  76764
        // BDD node 123:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__120) (ReadLSB w32 (w32 0) unrolled__134)), n:(w32 7))
        u32 rotated_64 = libnf::rotate_left((rotated_61) ^ (unrolled_100), 7);
        // EP node  77392
        // BDD node 366:op_xor(a:(ReadLSB w32 (w32 0) rotated__119), b:(ReadLSB w32 (w32 0) unrolled__132))
        u32 unrolled_101 = (rotated_60) ^ (unrolled_99);
        // EP node  78182
        // BDD node 124:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__134) (ReadLSB w32 (w32 0) unrolled__135)), n:(w32 16))
        u32 rotated_65 = libnf::rotate_left((unrolled_100) + (unrolled_101), 16);
        // EP node  78818
        // BDD node 367:op_add(a:(ReadLSB w32 (w32 0) unrolled__134), b:(ReadLSB w32 (w32 0) unrolled__135))
        u32 unrolled_102 = (unrolled_100) + (unrolled_101);
        // EP node  79618
        // BDD node 368:op_xor(a:(ReadLSB w32 (w32 0) rotated__120), b:(ReadLSB w32 (w32 0) unrolled__134))
        u32 unrolled_103 = (rotated_61) ^ (unrolled_100);
        // EP node  80423
        // BDD node 369:op_add(a:(ReadLSB w32 (w32 0) rotated__121), b:(ReadLSB w32 (w32 0) unrolled__137))
        u32 unrolled_104 = (rotated_62) + (unrolled_103);
        // EP node  81395
        // BDD node 370:op_xor(a:(ReadLSB w32 (w32 0) rotated__122), b:(ReadLSB w32 (w32 0) unrolled__136))
        u32 unrolled_105 = (rotated_63) ^ (unrolled_102);
        // EP node  82536
        // BDD node 372:op_xor(a:(ReadLSB w32 (w32 0) rotated__123), b:(ReadLSB w32 (w32 0) unrolled__138))
        u32 unrolled_106 = (rotated_64) ^ (unrolled_104);
        // EP node  83684
        // BDD node 125:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__122) (ReadLSB w32 (w32 0) unrolled__136)), n:(w32 5))
        u32 rotated_66 = libnf::rotate_left((rotated_63) ^ (unrolled_102), 5);
        // EP node  84674
        // BDD node 126:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__123) (ReadLSB w32 (w32 0) unrolled__138)), n:(w32 8))
        u32 rotated_67 = libnf::rotate_left((rotated_64) ^ (unrolled_104), 8);
        // EP node  85504
        // BDD node 127:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__138) (ReadLSB w32 (w32 0) unrolled__139)), n:(w32 16))
        u32 rotated_68 = libnf::rotate_left((unrolled_104) + (unrolled_105), 16);
        // EP node  86172
        // BDD node 371:op_add(a:(ReadLSB w32 (w32 0) unrolled__138), b:(ReadLSB w32 (w32 0) unrolled__139))
        u32 unrolled_107 = (unrolled_104) + (unrolled_105);
        // EP node  87012
        // BDD node 128:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__125) (ReadLSB w32 (w32 0) unrolled__140)), n:(w32 13))
        u32 rotated_69 = libnf::rotate_left((rotated_66) ^ (unrolled_107), 13);
        // EP node  87688
        // BDD node 373:op_add(a:(ReadLSB w32 (w32 0) rotated__124), b:(ReadLSB w32 (w32 0) unrolled__141))
        u32 unrolled_108 = (rotated_65) + (unrolled_106);
        // EP node  88538
        // BDD node 129:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__126) (ReadLSB w32 (w32 0) unrolled__142)), n:(w32 7))
        u32 rotated_70 = libnf::rotate_left((rotated_67) ^ (unrolled_108), 7);
        // EP node  89222
        // BDD node 374:op_xor(a:(ReadLSB w32 (w32 0) rotated__125), b:(ReadLSB w32 (w32 0) unrolled__140))
        u32 unrolled_109 = (rotated_66) ^ (unrolled_107);
        // EP node  90082
        // BDD node 130:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__142) (ReadLSB w32 (w32 0) unrolled__143)), n:(w32 16))
        u32 rotated_71 = libnf::rotate_left((unrolled_108) + (unrolled_109), 16);
        // EP node  90774
        // BDD node 375:op_add(a:(ReadLSB w32 (w32 0) unrolled__142), b:(ReadLSB w32 (w32 0) unrolled__143))
        u32 unrolled_110 = (unrolled_108) + (unrolled_109);
        // EP node  91644
        // BDD node 376:op_xor(a:(ReadLSB w32 (w32 0) rotated__126), b:(ReadLSB w32 (w32 0) unrolled__142))
        u32 unrolled_111 = (rotated_67) ^ (unrolled_108);
        // EP node  92519
        // BDD node 377:op_add(a:(ReadLSB w32 (w32 0) rotated__127), b:(ReadLSB w32 (w32 0) unrolled__145))
        u32 unrolled_112 = (rotated_68) + (unrolled_111);
        // EP node  93575
        // BDD node 378:op_xor(a:(ReadLSB w32 (w32 0) rotated__129), b:(ReadLSB w32 (w32 0) unrolled__146))
        u32 unrolled_113 = (rotated_70) ^ (unrolled_112);
        // EP node  94460
        // BDD node 382:op_xor(a:(ReadLSB w32 (w32 0) rotated__128), b:(ReadLSB w32 (w32 0) unrolled__144))
        u32 unrolled_114 = (rotated_69) ^ (unrolled_110);
        // EP node  95172
        // BDD node 379:op_shl(a:(ZExt w32 (ReadMSB w16 (w32 512) packet_chunks)), b:(w32 16))
        u32 unrolled_115 = ((u16)(bswap16(*(u16*)(hdr_5 + 0)))) << (16);
        // EP node  95888
        // BDD node 380:op_or(a:(ReadLSB w32 (w32 0) unrolled__148), b:(ZExt w32 (ReadMSB w16 (w32 514) packet_chunks)))
        u32 unrolled_116 = (unrolled_115) | ((u16)(bswap16(*(u16*)(hdr_5 + 2))));
        // EP node  96788
        // BDD node 381:op_xor(a:(ReadLSB w32 (w32 0) unrolled__146), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 272) packet_chunks) (Read w8 (w32 274) packet_chunks)) (Read w8 (w32 275) packet_chunks)))
        u32 unrolled_117 = (unrolled_112) ^ (bswap32(*(u32*)(hdr_4 + 16)));
        // EP node  97874
        // BDD node 384:op_xor(a:(ReadLSB w32 (w32 0) unrolled__147), b:(ReadLSB w32 (w32 0) unrolled__149))
        u32 unrolled_118 = (unrolled_113) ^ (unrolled_116);
        // EP node  98966
        // BDD node 131:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__128) (ReadLSB w32 (w32 0) unrolled__144)), n:(w32 5))
        u32 rotated_72 = libnf::rotate_left((rotated_69) ^ (unrolled_110), 5);
        // EP node  99881
        // BDD node 132:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) unrolled__147) (ReadLSB w32 (w32 0) unrolled__149)), n:(w32 8))
        u32 rotated_73 = libnf::rotate_left((unrolled_113) ^ (unrolled_116), 8);
        // EP node  100617
        // BDD node 133:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__150) (ReadLSB w32 (w32 0) unrolled__151)), n:(w32 16))
        u32 rotated_74 = libnf::rotate_left((unrolled_117) + (unrolled_114), 16);
        // EP node  101172
        // BDD node 383:op_add(a:(ReadLSB w32 (w32 0) unrolled__150), b:(ReadLSB w32 (w32 0) unrolled__151))
        u32 unrolled_119 = (unrolled_117) + (unrolled_114);
        // EP node  101916
        // BDD node 134:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__131) (ReadLSB w32 (w32 0) unrolled__152)), n:(w32 13))
        u32 rotated_75 = libnf::rotate_left((rotated_72) ^ (unrolled_119), 13);
        // EP node  102477
        // BDD node 385:op_add(a:(ReadLSB w32 (w32 0) rotated__130), b:(ReadLSB w32 (w32 0) unrolled__153))
        u32 unrolled_120 = (rotated_71) + (unrolled_118);
        // EP node  103229
        // BDD node 135:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__132) (ReadLSB w32 (w32 0) unrolled__154)), n:(w32 7))
        u32 rotated_76 = libnf::rotate_left((rotated_73) ^ (unrolled_120), 7);
        // EP node  103796
        // BDD node 386:op_xor(a:(ReadLSB w32 (w32 0) rotated__131), b:(ReadLSB w32 (w32 0) unrolled__152))
        u32 unrolled_121 = (rotated_72) ^ (unrolled_119);
        // EP node  104556
        // BDD node 136:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__154) (ReadLSB w32 (w32 0) unrolled__155)), n:(w32 16))
        u32 rotated_77 = libnf::rotate_left((unrolled_120) + (unrolled_121), 16);
        // EP node  105129
        // BDD node 387:op_add(a:(ReadLSB w32 (w32 0) unrolled__154), b:(ReadLSB w32 (w32 0) unrolled__155))
        u32 unrolled_122 = (unrolled_120) + (unrolled_121);
        // EP node  105897
        // BDD node 388:op_xor(a:(ReadLSB w32 (w32 0) rotated__132), b:(ReadLSB w32 (w32 0) unrolled__154))
        u32 unrolled_123 = (rotated_73) ^ (unrolled_120);
        // EP node  106669
        // BDD node 389:op_add(a:(ReadLSB w32 (w32 0) rotated__133), b:(ReadLSB w32 (w32 0) unrolled__157))
        u32 unrolled_124 = (rotated_74) + (unrolled_123);
        // EP node  107639
        // BDD node 390:op_xor(a:(ReadLSB w32 (w32 0) rotated__134), b:(ReadLSB w32 (w32 0) unrolled__156))
        u32 unrolled_125 = (rotated_75) ^ (unrolled_122);
        // EP node  108809
        // BDD node 392:op_xor(a:(ReadLSB w32 (w32 0) rotated__135), b:(ReadLSB w32 (w32 0) unrolled__158))
        u32 unrolled_126 = (rotated_76) ^ (unrolled_124);
        // EP node  109985
        // BDD node 137:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__134) (ReadLSB w32 (w32 0) unrolled__156)), n:(w32 5))
        u32 rotated_78 = libnf::rotate_left((rotated_75) ^ (unrolled_122), 5);
        // EP node  110970
        // BDD node 138:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__135) (ReadLSB w32 (w32 0) unrolled__158)), n:(w32 8))
        u32 rotated_79 = libnf::rotate_left((rotated_76) ^ (unrolled_124), 8);
        // EP node  111762
        // BDD node 139:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__158) (ReadLSB w32 (w32 0) unrolled__159)), n:(w32 16))
        u32 rotated_80 = libnf::rotate_left((unrolled_124) + (unrolled_125), 16);
        // EP node  112359
        // BDD node 391:op_add(a:(ReadLSB w32 (w32 0) unrolled__158), b:(ReadLSB w32 (w32 0) unrolled__159))
        u32 unrolled_127 = (unrolled_124) + (unrolled_125);
        // EP node  113159
        // BDD node 140:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__137) (ReadLSB w32 (w32 0) unrolled__160)), n:(w32 13))
        u32 rotated_81 = libnf::rotate_left((rotated_78) ^ (unrolled_127), 13);
        // EP node  113762
        // BDD node 393:op_add(a:(ReadLSB w32 (w32 0) rotated__136), b:(ReadLSB w32 (w32 0) unrolled__161))
        u32 unrolled_128 = (rotated_77) + (unrolled_126);
        // EP node  114570
        // BDD node 141:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__138) (ReadLSB w32 (w32 0) unrolled__162)), n:(w32 7))
        u32 rotated_82 = libnf::rotate_left((rotated_79) ^ (unrolled_128), 7);
        // EP node  115179
        // BDD node 394:op_xor(a:(ReadLSB w32 (w32 0) rotated__137), b:(ReadLSB w32 (w32 0) unrolled__160))
        u32 unrolled_129 = (rotated_78) ^ (unrolled_127);
        // EP node  115995
        // BDD node 142:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__162) (ReadLSB w32 (w32 0) unrolled__163)), n:(w32 16))
        u32 rotated_83 = libnf::rotate_left((unrolled_128) + (unrolled_129), 16);
        // EP node  116610
        // BDD node 395:op_add(a:(ReadLSB w32 (w32 0) unrolled__162), b:(ReadLSB w32 (w32 0) unrolled__163))
        u32 unrolled_130 = (unrolled_128) + (unrolled_129);
        // EP node  117434
        // BDD node 396:op_xor(a:(ReadLSB w32 (w32 0) rotated__138), b:(ReadLSB w32 (w32 0) unrolled__162))
        u32 unrolled_131 = (rotated_79) ^ (unrolled_128);
        // EP node  118262
        // BDD node 397:op_add(a:(ReadLSB w32 (w32 0) rotated__139), b:(ReadLSB w32 (w32 0) unrolled__165))
        u32 unrolled_132 = (rotated_80) + (unrolled_131);
        // EP node  119302
        // BDD node 398:op_xor(a:(ReadLSB w32 (w32 0) rotated__141), b:(ReadLSB w32 (w32 0) unrolled__166))
        u32 unrolled_133 = (rotated_82) ^ (unrolled_132);
        // EP node  120556
        // BDD node 400:op_xor(a:(ReadLSB w32 (w32 0) rotated__140), b:(ReadLSB w32 (w32 0) unrolled__164))
        u32 unrolled_134 = (rotated_81) ^ (unrolled_130);
        // EP node  121606
        // BDD node 399:op_xor(a:(ReadLSB w32 (w32 0) unrolled__166), b:(ReadLSB w32 (w32 0) unrolled__149))
        u32 unrolled_135 = (unrolled_132) ^ (unrolled_116);
        // EP node  122872
        // BDD node 402:op_xor(a:(ReadLSB w32 (w32 0) unrolled__167), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))
        u32 unrolled_136 = (unrolled_133) ^ (bswap32(*(u32*)(hdr_5 + 4)));
        // EP node  124144
        // BDD node 143:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__140) (ReadLSB w32 (w32 0) unrolled__164)), n:(w32 5))
        u32 rotated_84 = libnf::rotate_left((rotated_81) ^ (unrolled_130), 5);
        // EP node  125209
        // BDD node 144:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) unrolled__167) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks))), n:(w32 8))
        u32 rotated_85 = libnf::rotate_left((unrolled_133) ^ (bswap32(*(u32*)(hdr_5 + 4))), 8);
        // EP node  126065
        // BDD node 145:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__168) (ReadLSB w32 (w32 0) unrolled__169)), n:(w32 16))
        u32 rotated_86 = libnf::rotate_left((unrolled_135) + (unrolled_134), 16);
        // EP node  126710
        // BDD node 401:op_add(a:(ReadLSB w32 (w32 0) unrolled__168), b:(ReadLSB w32 (w32 0) unrolled__169))
        u32 unrolled_137 = (unrolled_135) + (unrolled_134);
        // EP node  127574
        // BDD node 146:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__143) (ReadLSB w32 (w32 0) unrolled__170)), n:(w32 13))
        u32 rotated_87 = libnf::rotate_left((rotated_84) ^ (unrolled_137), 13);
        // EP node  128225
        // BDD node 403:op_add(a:(ReadLSB w32 (w32 0) rotated__142), b:(ReadLSB w32 (w32 0) unrolled__171))
        u32 unrolled_138 = (rotated_83) + (unrolled_136);
        // EP node  129097
        // BDD node 147:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__144) (ReadLSB w32 (w32 0) unrolled__172)), n:(w32 7))
        u32 rotated_88 = libnf::rotate_left((rotated_85) ^ (unrolled_138), 7);
        // EP node  129754
        // BDD node 404:op_xor(a:(ReadLSB w32 (w32 0) rotated__143), b:(ReadLSB w32 (w32 0) unrolled__170))
        u32 unrolled_139 = (rotated_84) ^ (unrolled_137);
        // EP node  130634
        // BDD node 148:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__172) (ReadLSB w32 (w32 0) unrolled__173)), n:(w32 16))
        u32 rotated_89 = libnf::rotate_left((unrolled_138) + (unrolled_139), 16);
        // EP node  131297
        // BDD node 405:op_add(a:(ReadLSB w32 (w32 0) unrolled__172), b:(ReadLSB w32 (w32 0) unrolled__173))
        u32 unrolled_140 = (unrolled_138) + (unrolled_139);
        // EP node  132185
        // BDD node 406:op_xor(a:(ReadLSB w32 (w32 0) rotated__144), b:(ReadLSB w32 (w32 0) unrolled__172))
        u32 unrolled_141 = (rotated_85) ^ (unrolled_138);
        // EP node  133077
        // BDD node 407:op_add(a:(ReadLSB w32 (w32 0) rotated__145), b:(ReadLSB w32 (w32 0) unrolled__175))
        u32 unrolled_142 = (rotated_86) + (unrolled_141);
        // EP node  134197
        // BDD node 408:op_xor(a:(ReadLSB w32 (w32 0) rotated__146), b:(ReadLSB w32 (w32 0) unrolled__174))
        u32 unrolled_143 = (rotated_87) ^ (unrolled_140);
        // EP node  135547
        // BDD node 410:op_xor(a:(ReadLSB w32 (w32 0) rotated__147), b:(ReadLSB w32 (w32 0) unrolled__176))
        u32 unrolled_144 = (rotated_88) ^ (unrolled_142);
        // EP node  136903
        // BDD node 149:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__146) (ReadLSB w32 (w32 0) unrolled__174)), n:(w32 5))
        u32 rotated_90 = libnf::rotate_left((rotated_87) ^ (unrolled_140), 5);
        // EP node  138038
        // BDD node 150:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__147) (ReadLSB w32 (w32 0) unrolled__176)), n:(w32 8))
        u32 rotated_91 = libnf::rotate_left((rotated_88) ^ (unrolled_142), 8);
        // EP node  138950
        // BDD node 151:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__176) (ReadLSB w32 (w32 0) unrolled__177)), n:(w32 16))
        u32 rotated_92 = libnf::rotate_left((unrolled_142) + (unrolled_143), 16);
        // EP node  139637
        // BDD node 409:op_add(a:(ReadLSB w32 (w32 0) unrolled__176), b:(ReadLSB w32 (w32 0) unrolled__177))
        u32 unrolled_145 = (unrolled_142) + (unrolled_143);
        // EP node  140557
        // BDD node 152:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__149) (ReadLSB w32 (w32 0) unrolled__178)), n:(w32 13))
        u32 rotated_93 = libnf::rotate_left((rotated_90) ^ (unrolled_145), 13);
        // EP node  141250
        // BDD node 411:op_add(a:(ReadLSB w32 (w32 0) rotated__148), b:(ReadLSB w32 (w32 0) unrolled__179))
        u32 unrolled_146 = (rotated_89) + (unrolled_144);
        // EP node  142178
        // BDD node 153:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__150) (ReadLSB w32 (w32 0) unrolled__180)), n:(w32 7))
        u32 rotated_94 = libnf::rotate_left((rotated_91) ^ (unrolled_146), 7);
        // EP node  142877
        // BDD node 412:op_xor(a:(ReadLSB w32 (w32 0) rotated__149), b:(ReadLSB w32 (w32 0) unrolled__178))
        u32 unrolled_147 = (rotated_90) ^ (unrolled_145);
        // EP node  143813
        // BDD node 154:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__180) (ReadLSB w32 (w32 0) unrolled__181)), n:(w32 16))
        u32 rotated_95 = libnf::rotate_left((unrolled_146) + (unrolled_147), 16);
        // EP node  144518
        // BDD node 413:op_add(a:(ReadLSB w32 (w32 0) unrolled__180), b:(ReadLSB w32 (w32 0) unrolled__181))
        u32 unrolled_148 = (unrolled_146) + (unrolled_147);
        // EP node  145462
        // BDD node 414:op_xor(a:(ReadLSB w32 (w32 0) rotated__150), b:(ReadLSB w32 (w32 0) unrolled__180))
        u32 unrolled_149 = (rotated_91) ^ (unrolled_146);
        // EP node  146410
        // BDD node 415:op_add(a:(ReadLSB w32 (w32 0) rotated__151), b:(ReadLSB w32 (w32 0) unrolled__183))
        u32 unrolled_150 = (rotated_92) + (unrolled_149);
        // EP node  147838
        // BDD node 417:op_xor(a:(ReadLSB w32 (w32 0) rotated__152), b:(ReadLSB w32 (w32 0) unrolled__182))
        u32 unrolled_151 = (rotated_93) ^ (unrolled_148);
        // EP node  149033
        // BDD node 419:op_xor(a:(ReadLSB w32 (w32 0) rotated__153), b:(ReadLSB w32 (w32 0) unrolled__184))
        u32 unrolled_152 = (rotated_94) ^ (unrolled_150);
        // EP node  150233
        // BDD node 416:op_xor(a:(ReadLSB w32 (w32 0) unrolled__184), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))
        u32 unrolled_153 = (unrolled_150) ^ (bswap32(*(u32*)(hdr_5 + 4)));
        // EP node  151679
        // BDD node 155:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__152) (ReadLSB w32 (w32 0) unrolled__182)), n:(w32 5))
        u32 rotated_96 = libnf::rotate_left((rotated_93) ^ (unrolled_148), 5);
        // EP node  152889
        // BDD node 156:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__153) (ReadLSB w32 (w32 0) unrolled__184)), n:(w32 8))
        u32 rotated_97 = libnf::rotate_left((rotated_94) ^ (unrolled_150), 8);
        // EP node  153861
        // BDD node 157:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__185) (ReadLSB w32 (w32 0) unrolled__186)), n:(w32 16))
        u32 rotated_98 = libnf::rotate_left((unrolled_153) + (unrolled_151), 16);
        // EP node  154593
        // BDD node 418:op_add(a:(ReadLSB w32 (w32 0) unrolled__185), b:(ReadLSB w32 (w32 0) unrolled__186))
        u32 unrolled_154 = (unrolled_153) + (unrolled_151);
        // EP node  155573
        // BDD node 158:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__155) (ReadLSB w32 (w32 0) unrolled__187)), n:(w32 13))
        u32 rotated_99 = libnf::rotate_left((rotated_96) ^ (unrolled_154), 13);
        // EP node  156311
        // BDD node 420:op_add(a:(ReadLSB w32 (w32 0) rotated__154), b:(ReadLSB w32 (w32 0) unrolled__188))
        u32 unrolled_155 = (rotated_95) + (unrolled_152);
        // EP node  157299
        // BDD node 159:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__156) (ReadLSB w32 (w32 0) unrolled__189)), n:(w32 7))
        u32 rotated_100 = libnf::rotate_left((rotated_97) ^ (unrolled_155), 7);
        // EP node  158043
        // BDD node 421:op_xor(a:(ReadLSB w32 (w32 0) rotated__155), b:(ReadLSB w32 (w32 0) unrolled__187))
        u32 unrolled_156 = (rotated_96) ^ (unrolled_154);
        // EP node  159039
        // BDD node 160:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__189) (ReadLSB w32 (w32 0) unrolled__190)), n:(w32 16))
        u32 rotated_101 = libnf::rotate_left((unrolled_155) + (unrolled_156), 16);
        // EP node  159789
        // BDD node 422:op_add(a:(ReadLSB w32 (w32 0) unrolled__189), b:(ReadLSB w32 (w32 0) unrolled__190))
        u32 unrolled_157 = (unrolled_155) + (unrolled_156);
        // EP node  160793
        // BDD node 423:op_xor(a:(ReadLSB w32 (w32 0) rotated__156), b:(ReadLSB w32 (w32 0) unrolled__189))
        u32 unrolled_158 = (rotated_97) ^ (unrolled_155);
        // EP node  161801
        // BDD node 424:op_add(a:(ReadLSB w32 (w32 0) rotated__157), b:(ReadLSB w32 (w32 0) unrolled__192))
        u32 unrolled_159 = (rotated_98) + (unrolled_158);
        // EP node  163066
        // BDD node 425:op_xor(a:(ReadLSB w32 (w32 0) rotated__158), b:(ReadLSB w32 (w32 0) unrolled__191))
        u32 unrolled_160 = (rotated_99) ^ (unrolled_157);
        // EP node  164590
        // BDD node 427:op_xor(a:(ReadLSB w32 (w32 0) rotated__159), b:(ReadLSB w32 (w32 0) unrolled__193))
        u32 unrolled_161 = (rotated_100) ^ (unrolled_159);
        // EP node  166120
        // BDD node 161:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__158) (ReadLSB w32 (w32 0) unrolled__191)), n:(w32 5))
        u32 rotated_102 = libnf::rotate_left((rotated_99) ^ (unrolled_157), 5);
        // EP node  167400
        // BDD node 162:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__159) (ReadLSB w32 (w32 0) unrolled__193)), n:(w32 8))
        u32 rotated_103 = libnf::rotate_left((rotated_100) ^ (unrolled_159), 8);
        // EP node  168428
        // BDD node 163:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__193) (ReadLSB w32 (w32 0) unrolled__194)), n:(w32 16))
        u32 rotated_104 = libnf::rotate_left((unrolled_159) + (unrolled_160), 16);
        // EP node  169202
        // BDD node 426:op_add(a:(ReadLSB w32 (w32 0) unrolled__193), b:(ReadLSB w32 (w32 0) unrolled__194))
        u32 unrolled_162 = (unrolled_159) + (unrolled_160);
        // EP node  170238
        // BDD node 164:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__161) (ReadLSB w32 (w32 0) unrolled__195)), n:(w32 13))
        u32 rotated_105 = libnf::rotate_left((rotated_102) ^ (unrolled_162), 13);
        // EP node  171018
        // BDD node 428:op_add(a:(ReadLSB w32 (w32 0) rotated__160), b:(ReadLSB w32 (w32 0) unrolled__196))
        u32 unrolled_163 = (rotated_101) + (unrolled_161);
        // EP node  172062
        // BDD node 165:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__162) (ReadLSB w32 (w32 0) unrolled__197)), n:(w32 7))
        u32 rotated_106 = libnf::rotate_left((rotated_103) ^ (unrolled_163), 7);
        // EP node  172848
        // BDD node 429:op_xor(a:(ReadLSB w32 (w32 0) rotated__161), b:(ReadLSB w32 (w32 0) unrolled__195))
        u32 unrolled_164 = (rotated_102) ^ (unrolled_162);
        // EP node  173900
        // BDD node 166:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__197) (ReadLSB w32 (w32 0) unrolled__198)), n:(w32 16))
        u32 rotated_107 = libnf::rotate_left((unrolled_163) + (unrolled_164), 16);
        // EP node  174692
        // BDD node 430:op_add(a:(ReadLSB w32 (w32 0) unrolled__197), b:(ReadLSB w32 (w32 0) unrolled__198))
        u32 unrolled_165 = (unrolled_163) + (unrolled_164);
        // EP node  175752
        // BDD node 431:op_xor(a:(ReadLSB w32 (w32 0) rotated__162), b:(ReadLSB w32 (w32 0) unrolled__197))
        u32 unrolled_166 = (rotated_103) ^ (unrolled_163);
        // EP node  176816
        // BDD node 432:op_add(a:(ReadLSB w32 (w32 0) rotated__163), b:(ReadLSB w32 (w32 0) unrolled__200))
        u32 unrolled_167 = (rotated_104) + (unrolled_166);
        // EP node  178151
        // BDD node 433:op_xor(a:(ReadLSB w32 (w32 0) rotated__164), b:(ReadLSB w32 (w32 0) unrolled__199))
        u32 unrolled_168 = (rotated_105) ^ (unrolled_165);
        // EP node  179759
        // BDD node 435:op_xor(a:(ReadLSB w32 (w32 0) rotated__165), b:(ReadLSB w32 (w32 0) unrolled__201))
        u32 unrolled_169 = (rotated_106) ^ (unrolled_167);
        // EP node  181373
        // BDD node 167:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__164) (ReadLSB w32 (w32 0) unrolled__199)), n:(w32 5))
        u32 rotated_108 = libnf::rotate_left((rotated_105) ^ (unrolled_165), 5);
        // EP node  182723
        // BDD node 168:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__165) (ReadLSB w32 (w32 0) unrolled__201)), n:(w32 8))
        u32 rotated_109 = libnf::rotate_left((rotated_106) ^ (unrolled_167), 8);
        // EP node  183807
        // BDD node 169:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__201) (ReadLSB w32 (w32 0) unrolled__202)), n:(w32 16))
        u32 rotated_110 = libnf::rotate_left((unrolled_167) + (unrolled_168), 16);
        // EP node  184623
        // BDD node 434:op_add(a:(ReadLSB w32 (w32 0) unrolled__201), b:(ReadLSB w32 (w32 0) unrolled__202))
        u32 unrolled_170 = (unrolled_167) + (unrolled_168);
        // EP node  185715
        // BDD node 170:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__167) (ReadLSB w32 (w32 0) unrolled__203)), n:(w32 13))
        u32 rotated_111 = libnf::rotate_left((rotated_108) ^ (unrolled_170), 13);
        // EP node  186537
        // BDD node 436:op_add(a:(ReadLSB w32 (w32 0) rotated__166), b:(ReadLSB w32 (w32 0) unrolled__204))
        u32 unrolled_171 = (rotated_107) + (unrolled_169);
        // EP node  187637
        // BDD node 171:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__168) (ReadLSB w32 (w32 0) unrolled__205)), n:(w32 7))
        u32 rotated_112 = libnf::rotate_left((rotated_109) ^ (unrolled_171), 7);
        // EP node  188465
        // BDD node 437:op_xor(a:(ReadLSB w32 (w32 0) rotated__167), b:(ReadLSB w32 (w32 0) unrolled__203))
        u32 unrolled_172 = (rotated_108) ^ (unrolled_170);
        // EP node  189573
        // BDD node 172:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__205) (ReadLSB w32 (w32 0) unrolled__206)), n:(w32 16))
        u32 rotated_113 = libnf::rotate_left((unrolled_171) + (unrolled_172), 16);
        // EP node  190407
        // BDD node 438:op_add(a:(ReadLSB w32 (w32 0) unrolled__205), b:(ReadLSB w32 (w32 0) unrolled__206))
        u32 unrolled_173 = (unrolled_171) + (unrolled_172);
        // EP node  191523
        // BDD node 439:op_xor(a:(ReadLSB w32 (w32 0) rotated__168), b:(ReadLSB w32 (w32 0) unrolled__205))
        u32 unrolled_174 = (rotated_109) ^ (unrolled_171);
        // EP node  192643
        // BDD node 440:op_add(a:(ReadLSB w32 (w32 0) rotated__169), b:(ReadLSB w32 (w32 0) unrolled__208))
        u32 unrolled_175 = (rotated_110) + (unrolled_174);
        // EP node  194048
        // BDD node 441:op_xor(a:(ReadLSB w32 (w32 0) rotated__170), b:(ReadLSB w32 (w32 0) unrolled__207))
        u32 unrolled_176 = (rotated_111) ^ (unrolled_173);
        // EP node  195740
        // BDD node 443:op_xor(a:(ReadLSB w32 (w32 0) rotated__171), b:(ReadLSB w32 (w32 0) unrolled__209))
        u32 unrolled_177 = (rotated_112) ^ (unrolled_175);
        // EP node  197438
        // BDD node 173:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__170) (ReadLSB w32 (w32 0) unrolled__207)), n:(w32 5))
        u32 rotated_114 = libnf::rotate_left((rotated_111) ^ (unrolled_173), 5);
        // EP node  198858
        // BDD node 174:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__171) (ReadLSB w32 (w32 0) unrolled__209)), n:(w32 8))
        u32 rotated_115 = libnf::rotate_left((rotated_112) ^ (unrolled_175), 8);
        // EP node  199998
        // BDD node 175:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__209) (ReadLSB w32 (w32 0) unrolled__210)), n:(w32 16))
        u32 rotated_116 = libnf::rotate_left((unrolled_175) + (unrolled_176), 16);
        // EP node  200856
        // BDD node 442:op_add(a:(ReadLSB w32 (w32 0) unrolled__209), b:(ReadLSB w32 (w32 0) unrolled__210))
        u32 unrolled_178 = (unrolled_175) + (unrolled_176);
        // EP node  202004
        // BDD node 176:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__173) (ReadLSB w32 (w32 0) unrolled__211)), n:(w32 13))
        u32 rotated_117 = libnf::rotate_left((rotated_114) ^ (unrolled_178), 13);
        // EP node  202868
        // BDD node 444:op_add(a:(ReadLSB w32 (w32 0) rotated__172), b:(ReadLSB w32 (w32 0) unrolled__212))
        u32 unrolled_179 = (rotated_113) + (unrolled_177);
        // EP node  204024
        // BDD node 177:rotate_left(x:(Xor w32 (ReadLSB w32 (w32 0) rotated__174) (ReadLSB w32 (w32 0) unrolled__213)), n:(w32 7))
        u32 rotated_118 = libnf::rotate_left((rotated_115) ^ (unrolled_179), 7);
        // EP node  204894
        // BDD node 445:op_xor(a:(ReadLSB w32 (w32 0) rotated__173), b:(ReadLSB w32 (w32 0) unrolled__211))
        u32 unrolled_180 = (rotated_114) ^ (unrolled_178);
        // EP node  206058
        // BDD node 178:rotate_left(x:(Add w32 (ReadLSB w32 (w32 0) unrolled__213) (ReadLSB w32 (w32 0) unrolled__214)), n:(w32 16))
        u32 rotated_119 = libnf::rotate_left((unrolled_179) + (unrolled_180), 16);
        // EP node  207518
        // BDD node 179:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073757248), l4_header:(w64 1073757504), packet:(w64 1073957496))
        trigger_update_ipv4_tcpudp_checksums = true;
        l3_hdr = (void *)hdr_4;
        l4_hdr = (void *)hdr_5;
        // EP node  208105
        // BDD node 446:op_lshr(a:(ReadLSB w64 (w32 0) next_time), b:(w64 16))
        u64 unrolled_181 = (u64)((now>>16) & 0xffffffffffffull);
        // EP node  208987
        // BDD node 447:op_sub(a:(ReadLSB w32 (w32 0) unrolled__215), b:(ReadLSB w32 (w32 0) vector_data__105))
        u32 unrolled_182 = (unrolled_181 & 0xffffffffull) - (bswap32(cpu_hdr_extra->vector_data__105));
        // EP node  209872
        // BDD node 448:op_lshr(a:(ReadLSB w32 (w32 0) unrolled__216), b:(w32 12))
        u32 unrolled_183 = (unrolled_182) >> (12);
        // EP node  210464
        // BDD node 449:op_xor(a:(ReadLSB w32 (w32 0) rotated__174), b:(ReadLSB w32 (w32 0) unrolled__213))
        u32 unrolled_184 = (rotated_115) ^ (unrolled_179);
        // EP node  211058
        // BDD node 450:op_add(a:(ReadLSB w32 (w32 0) rotated__175), b:(ReadLSB w32 (w32 0) unrolled__218))
        u32 unrolled_185 = (rotated_116) + (unrolled_184);
        // EP node  211654
        // BDD node 451:op_add(a:(ReadLSB w32 (w32 0) unrolled__213), b:(ReadLSB w32 (w32 0) unrolled__214))
        u32 unrolled_186 = (unrolled_179) + (unrolled_180);
        // EP node  212252
        // BDD node 452:op_xor(a:(ReadLSB w32 (w32 0) rotated__176), b:(ReadLSB w32 (w32 0) unrolled__220))
        u32 unrolled_187 = (rotated_117) ^ (unrolled_186);
        // EP node  212852
        // BDD node 453:op_xor(a:(ReadLSB w32 (w32 0) unrolled__219), b:(ReadLSB w32 (w32 0) unrolled__221))
        u32 unrolled_188 = (unrolled_185) ^ (unrolled_187);
        // EP node  213454
        // BDD node 454:op_xor(a:(ReadLSB w32 (w32 0) unrolled__222), b:(ReadLSB w32 (w32 0) rotated__178))
        u32 unrolled_189 = (unrolled_188) ^ (rotated_119);
        // EP node  213756
        // BDD node 455:op_xor(a:(ReadLSB w32 (w32 0) rotated__177), b:(ReadLSB w32 (w32 0) unrolled__219))
        u32 unrolled_190 = (rotated_118) ^ (unrolled_185);
        // EP node  214059
        // BDD node 456:op_xor(a:(ReadLSB w32 (w32 0) unrolled__223), b:(ReadLSB w32 (w32 0) unrolled__224))
        u32 unrolled_191 = (unrolled_189) ^ (unrolled_190);
        // EP node  214667
        // BDD node 180:packet_return_chunk(p:(w64 1074032776), the_chunk:(w64 1073757504)[(Concat w160 (Read w8 (w32 531) packet_chunks) (Concat w152 (Read w8 (w32 530) packet_chunks) (Concat w144 (Read w8 (w32 529) packet_chunks) (Concat w136 (Read w8 (w32 528) packet_chunks) (Concat w128 (Read w8 (w32 527) packet_chunks) (Concat w120 (Read w8 (w32 526) packet_chunks) (Concat w112 (Extract w8 0 (Or w32 (ZExt w32 (Read w8 (w32 525) packet_chunks)) (w32 18))) (Concat w104 (w8 80) (Concat w96 (Extract w8 0 (Add w32 (w32 1) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w88 (Extract w8 8 (Add w32 (w32 1) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w80 (Extract w8 16 (Add w32 (w32 1) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w72 (Extract w8 24 (Add w32 (w32 1) (Concat w32 (Concat w24 (ReadMSB w16 (w32 516) packet_chunks) (Read w8 (w32 518) packet_chunks)) (Read w8 (w32 519) packet_chunks)))) (Concat w64 (Extract w8 0 (Xor w32 (ReadLSB w32 (w32 0) unrolled__217) (ReadLSB w32 (w32 0) unrolled__225))) (Concat w56 (Extract w8 8 (Xor w32 (ReadLSB w32 (w32 0) unrolled__217) (ReadLSB w32 (w32 0) unrolled__225))) (Concat w48 (Extract w8 16 (Xor w32 (ReadLSB w32 (w32 0) unrolled__217) (ReadLSB w32 (w32 0) unrolled__225))) (Concat w40 (Extract w8 24 (Xor w32 (ReadLSB w32 (w32 0) unrolled__217) (ReadLSB w32 (w32 0) unrolled__225))) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))))))))))))))])
        const u8 hdr_5_214667_b4 = (u8)(((unrolled_183) ^ (unrolled_191))>>24);
        const u8 hdr_5_214667_b5 = (u8)(((unrolled_183) ^ (unrolled_191))>>16);
        const u8 hdr_5_214667_b6 = (u8)(((unrolled_183) ^ (unrolled_191))>>8);
        const u8 hdr_5_214667_b7 = (u8)(((unrolled_183) ^ (unrolled_191)));
        const u8 hdr_5_214667_b8 = (u8)(((1) + (bswap32(*(u32*)(hdr_5 + 4))))>>24);
        const u8 hdr_5_214667_b9 = (u8)(((1) + (bswap32(*(u32*)(hdr_5 + 4))))>>16);
        const u8 hdr_5_214667_b10 = (u8)(((1) + (bswap32(*(u32*)(hdr_5 + 4))))>>8);
        const u8 hdr_5_214667_b11 = (u8)(((1) + (bswap32(*(u32*)(hdr_5 + 4)))));
        const u8 hdr_5_214667_b12 = 80;
        const u8 hdr_5_214667_b13 = (u8)((((u8)(*(u8*)(hdr_5 + 13))) | (18)));
        std::swap(hdr_5[0], hdr_5[2]);
        std::swap(hdr_5[1], hdr_5[3]);
        hdr_5[4] = hdr_5_214667_b4;
        hdr_5[5] = hdr_5_214667_b5;
        hdr_5[6] = hdr_5_214667_b6;
        hdr_5[7] = hdr_5_214667_b7;
        hdr_5[8] = hdr_5_214667_b8;
        hdr_5[9] = hdr_5_214667_b9;
        hdr_5[10] = hdr_5_214667_b10;
        hdr_5[11] = hdr_5_214667_b11;
        hdr_5[12] = hdr_5_214667_b12;
        hdr_5[13] = hdr_5_214667_b13;
        // EP node  214973
        // BDD node 181:packet_return_chunk(p:(w64 1074032776), the_chunk:(w64 1073757248)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (Concat w96 (Read w8 (w32 1) checksum__179) (Concat w88 (Read w8 (w32 0) checksum__179) (Concat w80 (Read w8 (w32 265) packet_chunks) (Concat w72 (Read w8 (w32 264) packet_chunks) (Concat w64 (Read w8 (w32 263) packet_chunks) (Concat w56 (Read w8 (w32 262) packet_chunks) (Concat w48 (Read w8 (w32 261) packet_chunks) (Concat w40 (Read w8 (w32 260) packet_chunks) (Concat w32 (w8 40) (Concat w24 (w8 0) (Concat w16 (Read w8 (w32 257) packet_chunks) (w8 69))))))))))))))))))))])
        const u8 hdr_4_214973_b0 = 69;
        const u8 hdr_4_214973_b2 = 0;
        const u8 hdr_4_214973_b3 = 40;
        std::swap(hdr_4[12], hdr_4[16]);
        std::swap(hdr_4[13], hdr_4[17]);
        std::swap(hdr_4[14], hdr_4[18]);
        std::swap(hdr_4[15], hdr_4[19]);
        hdr_4[0] = hdr_4_214973_b0;
        hdr_4[2] = hdr_4_214973_b2;
        hdr_4[3] = hdr_4_214973_b3;
        // EP node  215586
        // BDD node 183:FORWARD
        cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull);
      } else {
        // EP node  14043
        // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
        // EP node  14044
        // BDD node 361:op_xor(a:(ReadLSB w32 (w32 0) unrolled__128), b:(Concat w32 (Concat w24 (ReadMSB w16 (w32 268) packet_chunks) (Read w8 (w32 270) packet_chunks)) (Read w8 (w32 271) packet_chunks)))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    }
  }
  else if (bswap16(cpu_hdr->code_path) == 2) {
    // EP node  70218
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_6 = packet_consume(pkt, 14);
    // EP node  70219
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_7 = packet_consume(pkt, 20);
    // EP node  70220
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_8 = packet_consume(pkt, 8);
    // EP node  70221
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_9 = packet_consume(pkt, 4);
    // EP node  551085
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    buffer_t vector_register_1073939616_value_0(4);
    vector_register_1073939616_value_0.set(0, 4, (bswap64(cpu_hdr_extra->unrolled__226) & 0xffffffffull) - (bswap32(*(u32*)hdr_9)));
    state->vector_register_1073939616.put(0, vector_register_1073939616_value_0);
    // EP node  552986
    // BDD node 217:DROP
    result.forward = false;
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
