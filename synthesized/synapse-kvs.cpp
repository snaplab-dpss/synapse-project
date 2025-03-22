#include <sycon/sycon.h>

using namespace sycon;

struct state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  MapTable map_table_1073923128;
  VectorRegister vector_register_1073956208;
  DchainTable dchain_table_1073989720;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      map_table_1073923128({"Ingress.map_table_1073923128_13",}, 1000LL),
      vector_register_1073956208({"Ingress.vector_register_1073956208_0","Ingress.vector_register_1073956208_1","Ingress.vector_register_1073956208_2","Ingress.vector_register_1073956208_3","Ingress.vector_register_1073956208_4","Ingress.vector_register_1073956208_5","Ingress.vector_register_1073956208_6","Ingress.vector_register_1073956208_7","Ingress.vector_register_1073956208_8","Ingress.vector_register_1073956208_9","Ingress.vector_register_1073956208_10","Ingress.vector_register_1073956208_11","Ingress.vector_register_1073956208_12","Ingress.vector_register_1073956208_13","Ingress.vector_register_1073956208_14","Ingress.vector_register_1073956208_15","Ingress.vector_register_1073956208_16","Ingress.vector_register_1073956208_17","Ingress.vector_register_1073956208_18","Ingress.vector_register_1073956208_19","Ingress.vector_register_1073956208_20","Ingress.vector_register_1073956208_21","Ingress.vector_register_1073956208_22","Ingress.vector_register_1073956208_23","Ingress.vector_register_1073956208_24","Ingress.vector_register_1073956208_25","Ingress.vector_register_1073956208_26","Ingress.vector_register_1073956208_27","Ingress.vector_register_1073956208_28","Ingress.vector_register_1073956208_29","Ingress.vector_register_1073956208_30","Ingress.vector_register_1073956208_31",}),
      dchain_table_1073989720({"Ingress.dchain_table_1073989720_38",}, 1000LL)
    {}
};

std::unique_ptr<state_t> state;

void sycon::nf_init() {
  state = std::make_unique<state_t>();
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(1), 0);
  state->forward_nf_dev.add_entry(0, asic_get_dev_port(1));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(3), 1);
  state->forward_nf_dev.add_entry(1, asic_get_dev_port(3));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(4), 1);
  state->forward_nf_dev.add_entry(1, asic_get_dev_port(4));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(5), 2);
  state->forward_nf_dev.add_entry(2, asic_get_dev_port(5));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(6), 3);
  state->forward_nf_dev.add_entry(3, asic_get_dev_port(6));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(7), 4);
  state->forward_nf_dev.add_entry(4, asic_get_dev_port(7));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(8), 5);
  state->forward_nf_dev.add_entry(5, asic_get_dev_port(8));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(9), 6);
  state->forward_nf_dev.add_entry(6, asic_get_dev_port(9));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(10), 7);
  state->forward_nf_dev.add_entry(7, asic_get_dev_port(10));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(11), 8);
  state->forward_nf_dev.add_entry(8, asic_get_dev_port(11));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(12), 9);
  state->forward_nf_dev.add_entry(9, asic_get_dev_port(12));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(13), 10);
  state->forward_nf_dev.add_entry(10, asic_get_dev_port(13));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(14), 11);
  state->forward_nf_dev.add_entry(11, asic_get_dev_port(14));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(15), 12);
  state->forward_nf_dev.add_entry(12, asic_get_dev_port(15));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(16), 13);
  state->forward_nf_dev.add_entry(13, asic_get_dev_port(16));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(17), 14);
  state->forward_nf_dev.add_entry(14, asic_get_dev_port(17));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(18), 15);
  state->forward_nf_dev.add_entry(15, asic_get_dev_port(18));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(19), 16);
  state->forward_nf_dev.add_entry(16, asic_get_dev_port(19));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(20), 17);
  state->forward_nf_dev.add_entry(17, asic_get_dev_port(20));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(21), 18);
  state->forward_nf_dev.add_entry(18, asic_get_dev_port(21));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(22), 19);
  state->forward_nf_dev.add_entry(19, asic_get_dev_port(22));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(23), 20);
  state->forward_nf_dev.add_entry(20, asic_get_dev_port(23));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(24), 21);
  state->forward_nf_dev.add_entry(21, asic_get_dev_port(24));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(25), 22);
  state->forward_nf_dev.add_entry(22, asic_get_dev_port(25));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(26), 23);
  state->forward_nf_dev.add_entry(23, asic_get_dev_port(26));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(27), 24);
  state->forward_nf_dev.add_entry(24, asic_get_dev_port(27));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(28), 25);
  state->forward_nf_dev.add_entry(25, asic_get_dev_port(28));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(29), 26);
  state->forward_nf_dev.add_entry(26, asic_get_dev_port(29));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(30), 27);
  state->forward_nf_dev.add_entry(27, asic_get_dev_port(30));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(31), 28);
  state->forward_nf_dev.add_entry(28, asic_get_dev_port(31));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(32), 29);
  state->forward_nf_dev.add_entry(29, asic_get_dev_port(32));
  // BDD node 0:map_allocate(capacity:(w32 8192), key_size:(w32 16), map_out:(w64 1073922856)[(w64 0) -> (w64 1073923128)])
  // Module MapTableAllocate
  // BDD node 2:vector_allocate(elem_size:(w32 128), capacity:(w32 8192), vector_out:(w64 1073922872)[(w64 0) -> (w64 1073956208)])
  // Module VectorRegisterAllocate
  // BDD node 3:dchain_allocate(index_range:(w32 8192), chain_out:(w64 1073922880)[ -> (w64 1073989720)])
  // Module DchainTableAllocate

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 DEVICE;

} __attribute__((packed));

bool sycon::nf_process(time_ns_t now, u8 *pkt, u16 size) {
  bool forward = true;
  bool trigger_update_ipv4_tcpudp_checksums = false;
  void* l3_hdr = nullptr;
  void* l4_hdr = nullptr;

  cpu_hdr_t *cpu_hdr = packet_consume<cpu_hdr_t>(pkt);
  cpu_hdr_extra_t *cpu_hdr_extra = packet_consume<cpu_hdr_extra_t>(pkt);
  LOG_DEBUG("[t=%lu] New packet (size=%u, code_path=%d)\n", now, size, bswap16(cpu_hdr->code_path));

  if (bswap16(cpu_hdr->code_path) == 6934) {
    // EP node  9267
    // BDD node 138:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 14), chunk:(w64 1074060080)[ -> (w64 1073759488)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  9381
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073989720), index_out:(w64 1074063120)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index_r7)], time:(ReadLSB w64 (w32 0) next_time))
    u32 allocated_index_0;
    bool success_0 = state->dchain_table_1073989720.allocate_new_index(allocated_index_0);
    // EP node  9555
    // BDD node 17:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_r7))
    if ((0) == (success_0)) {
      // EP node  9556
      // BDD node 17:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_r7))
      // EP node  10479
      // BDD node 139:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 20), chunk:(w64 1074060816)[ -> (w64 1073759744)])
      u8* hdr_1 = packet_consume(pkt, 20);
      // EP node  10552
      // BDD node 140:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 8), chunk:(w64 1074061472)[ -> (w64 1073760000)])
      u8* hdr_2 = packet_consume(pkt, 8);
      // EP node  10626
      // BDD node 141:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 148), chunk:(w64 1074062200)[ -> (w64 1073760256)])
      u8* hdr_3 = packet_consume(pkt, 148);
      // EP node  10776
      // BDD node 18:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 1) DEVICE) (Concat w1176 (Read w8 (w32 0) DEVICE) (Concat w1168 (w8 0) (ReadLSB w1160 (w32 768) packet_chunks))))])
      hdr_3[145] = 0;
      hdr_3[146] = bswap32(cpu_hdr_extra->DEVICE) & 255;
      hdr_3[147] = (bswap32(cpu_hdr_extra->DEVICE)>>8) & 255;
      // EP node  11081
      // BDD node 22:FORWARD
      cpu_hdr->egress_dev = bswap16(0);
    } else {
      // EP node  9557
      // BDD node 17:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_r7))
      // EP node  9677
      // BDD node 26:vector_borrow(vector:(w64 1073956208), index:(ReadLSB w32 (w32 0) new_index_r7), val_out:(w64 1074063176)[ -> (w64 1073970104)])
      // EP node  9739
      // BDD node 142:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 20), chunk:(w64 1074060816)[ -> (w64 1073759744)])
      u8* hdr_4 = packet_consume(pkt, 20);
      // EP node  9802
      // BDD node 143:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 8), chunk:(w64 1074061472)[ -> (w64 1073760000)])
      u8* hdr_5 = packet_consume(pkt, 8);
      // EP node  9866
      // BDD node 144:packet_borrow_next_chunk(p:(w64 1074049376), length:(w32 148), chunk:(w64 1074062200)[ -> (w64 1073760256)])
      u8* hdr_6 = packet_consume(pkt, 148);
      // EP node  9996
      // BDD node 24:map_put(map:(w64 1073923128), key:(w64 1073950848)[(ReadLSB w128 (w32 769) packet_chunks) -> (ReadLSB w128 (w32 769) packet_chunks)], value:(ReadLSB w32 (w32 0) new_index_r7))
      buffer_t map_table_1073923128_key_0(16);
      map_table_1073923128_key_0[0] = *(u8*)(hdr_6 + 1);
      map_table_1073923128_key_0[1] = *(u8*)(hdr_6 + 2);
      map_table_1073923128_key_0[2] = *(u8*)(hdr_6 + 3);
      map_table_1073923128_key_0[3] = *(u8*)(hdr_6 + 4);
      map_table_1073923128_key_0[4] = *(u8*)(hdr_6 + 5);
      map_table_1073923128_key_0[5] = *(u8*)(hdr_6 + 6);
      map_table_1073923128_key_0[6] = *(u8*)(hdr_6 + 7);
      map_table_1073923128_key_0[7] = *(u8*)(hdr_6 + 8);
      map_table_1073923128_key_0[8] = *(u8*)(hdr_6 + 9);
      map_table_1073923128_key_0[9] = *(u8*)(hdr_6 + 10);
      map_table_1073923128_key_0[10] = *(u8*)(hdr_6 + 11);
      map_table_1073923128_key_0[11] = *(u8*)(hdr_6 + 12);
      map_table_1073923128_key_0[12] = *(u8*)(hdr_6 + 13);
      map_table_1073923128_key_0[13] = *(u8*)(hdr_6 + 14);
      map_table_1073923128_key_0[14] = *(u8*)(hdr_6 + 15);
      map_table_1073923128_key_0[15] = *(u8*)(hdr_6 + 16);
      state->map_table_1073923128.put(map_table_1073923128_key_0, allocated_index_0);
      // EP node  10128
      // BDD node 27:vector_return(vector:(w64 1073956208), index:(ReadLSB w32 (w32 0) new_index_r7), value:(w64 1073970104)[(ReadLSB w1024 (w32 785) packet_chunks)])
      buffer_t vector_register_1073956208_value_0(128);
      vector_register_1073956208_value_0[0] = *(u8*)(hdr_6 + 17);
      vector_register_1073956208_value_0[1] = *(u8*)(hdr_6 + 18);
      vector_register_1073956208_value_0[2] = *(u8*)(hdr_6 + 19);
      vector_register_1073956208_value_0[3] = *(u8*)(hdr_6 + 20);
      vector_register_1073956208_value_0[4] = *(u8*)(hdr_6 + 21);
      vector_register_1073956208_value_0[5] = *(u8*)(hdr_6 + 22);
      vector_register_1073956208_value_0[6] = *(u8*)(hdr_6 + 23);
      vector_register_1073956208_value_0[7] = *(u8*)(hdr_6 + 24);
      vector_register_1073956208_value_0[8] = *(u8*)(hdr_6 + 25);
      vector_register_1073956208_value_0[9] = *(u8*)(hdr_6 + 26);
      vector_register_1073956208_value_0[10] = *(u8*)(hdr_6 + 27);
      vector_register_1073956208_value_0[11] = *(u8*)(hdr_6 + 28);
      vector_register_1073956208_value_0[12] = *(u8*)(hdr_6 + 29);
      vector_register_1073956208_value_0[13] = *(u8*)(hdr_6 + 30);
      vector_register_1073956208_value_0[14] = *(u8*)(hdr_6 + 31);
      vector_register_1073956208_value_0[15] = *(u8*)(hdr_6 + 32);
      vector_register_1073956208_value_0[16] = *(u8*)(hdr_6 + 33);
      vector_register_1073956208_value_0[17] = *(u8*)(hdr_6 + 34);
      vector_register_1073956208_value_0[18] = *(u8*)(hdr_6 + 35);
      vector_register_1073956208_value_0[19] = *(u8*)(hdr_6 + 36);
      vector_register_1073956208_value_0[20] = *(u8*)(hdr_6 + 37);
      vector_register_1073956208_value_0[21] = *(u8*)(hdr_6 + 38);
      vector_register_1073956208_value_0[22] = *(u8*)(hdr_6 + 39);
      vector_register_1073956208_value_0[23] = *(u8*)(hdr_6 + 40);
      vector_register_1073956208_value_0[24] = *(u8*)(hdr_6 + 41);
      vector_register_1073956208_value_0[25] = *(u8*)(hdr_6 + 42);
      vector_register_1073956208_value_0[26] = *(u8*)(hdr_6 + 43);
      vector_register_1073956208_value_0[27] = *(u8*)(hdr_6 + 44);
      vector_register_1073956208_value_0[28] = *(u8*)(hdr_6 + 45);
      vector_register_1073956208_value_0[29] = *(u8*)(hdr_6 + 46);
      vector_register_1073956208_value_0[30] = *(u8*)(hdr_6 + 47);
      vector_register_1073956208_value_0[31] = *(u8*)(hdr_6 + 48);
      vector_register_1073956208_value_0[32] = *(u8*)(hdr_6 + 49);
      vector_register_1073956208_value_0[33] = *(u8*)(hdr_6 + 50);
      vector_register_1073956208_value_0[34] = *(u8*)(hdr_6 + 51);
      vector_register_1073956208_value_0[35] = *(u8*)(hdr_6 + 52);
      vector_register_1073956208_value_0[36] = *(u8*)(hdr_6 + 53);
      vector_register_1073956208_value_0[37] = *(u8*)(hdr_6 + 54);
      vector_register_1073956208_value_0[38] = *(u8*)(hdr_6 + 55);
      vector_register_1073956208_value_0[39] = *(u8*)(hdr_6 + 56);
      vector_register_1073956208_value_0[40] = *(u8*)(hdr_6 + 57);
      vector_register_1073956208_value_0[41] = *(u8*)(hdr_6 + 58);
      vector_register_1073956208_value_0[42] = *(u8*)(hdr_6 + 59);
      vector_register_1073956208_value_0[43] = *(u8*)(hdr_6 + 60);
      vector_register_1073956208_value_0[44] = *(u8*)(hdr_6 + 61);
      vector_register_1073956208_value_0[45] = *(u8*)(hdr_6 + 62);
      vector_register_1073956208_value_0[46] = *(u8*)(hdr_6 + 63);
      vector_register_1073956208_value_0[47] = *(u8*)(hdr_6 + 64);
      vector_register_1073956208_value_0[48] = *(u8*)(hdr_6 + 65);
      vector_register_1073956208_value_0[49] = *(u8*)(hdr_6 + 66);
      vector_register_1073956208_value_0[50] = *(u8*)(hdr_6 + 67);
      vector_register_1073956208_value_0[51] = *(u8*)(hdr_6 + 68);
      vector_register_1073956208_value_0[52] = *(u8*)(hdr_6 + 69);
      vector_register_1073956208_value_0[53] = *(u8*)(hdr_6 + 70);
      vector_register_1073956208_value_0[54] = *(u8*)(hdr_6 + 71);
      vector_register_1073956208_value_0[55] = *(u8*)(hdr_6 + 72);
      vector_register_1073956208_value_0[56] = *(u8*)(hdr_6 + 73);
      vector_register_1073956208_value_0[57] = *(u8*)(hdr_6 + 74);
      vector_register_1073956208_value_0[58] = *(u8*)(hdr_6 + 75);
      vector_register_1073956208_value_0[59] = *(u8*)(hdr_6 + 76);
      vector_register_1073956208_value_0[60] = *(u8*)(hdr_6 + 77);
      vector_register_1073956208_value_0[61] = *(u8*)(hdr_6 + 78);
      vector_register_1073956208_value_0[62] = *(u8*)(hdr_6 + 79);
      vector_register_1073956208_value_0[63] = *(u8*)(hdr_6 + 80);
      vector_register_1073956208_value_0[64] = *(u8*)(hdr_6 + 81);
      vector_register_1073956208_value_0[65] = *(u8*)(hdr_6 + 82);
      vector_register_1073956208_value_0[66] = *(u8*)(hdr_6 + 83);
      vector_register_1073956208_value_0[67] = *(u8*)(hdr_6 + 84);
      vector_register_1073956208_value_0[68] = *(u8*)(hdr_6 + 85);
      vector_register_1073956208_value_0[69] = *(u8*)(hdr_6 + 86);
      vector_register_1073956208_value_0[70] = *(u8*)(hdr_6 + 87);
      vector_register_1073956208_value_0[71] = *(u8*)(hdr_6 + 88);
      vector_register_1073956208_value_0[72] = *(u8*)(hdr_6 + 89);
      vector_register_1073956208_value_0[73] = *(u8*)(hdr_6 + 90);
      vector_register_1073956208_value_0[74] = *(u8*)(hdr_6 + 91);
      vector_register_1073956208_value_0[75] = *(u8*)(hdr_6 + 92);
      vector_register_1073956208_value_0[76] = *(u8*)(hdr_6 + 93);
      vector_register_1073956208_value_0[77] = *(u8*)(hdr_6 + 94);
      vector_register_1073956208_value_0[78] = *(u8*)(hdr_6 + 95);
      vector_register_1073956208_value_0[79] = *(u8*)(hdr_6 + 96);
      vector_register_1073956208_value_0[80] = *(u8*)(hdr_6 + 97);
      vector_register_1073956208_value_0[81] = *(u8*)(hdr_6 + 98);
      vector_register_1073956208_value_0[82] = *(u8*)(hdr_6 + 99);
      vector_register_1073956208_value_0[83] = *(u8*)(hdr_6 + 100);
      vector_register_1073956208_value_0[84] = *(u8*)(hdr_6 + 101);
      vector_register_1073956208_value_0[85] = *(u8*)(hdr_6 + 102);
      vector_register_1073956208_value_0[86] = *(u8*)(hdr_6 + 103);
      vector_register_1073956208_value_0[87] = *(u8*)(hdr_6 + 104);
      vector_register_1073956208_value_0[88] = *(u8*)(hdr_6 + 105);
      vector_register_1073956208_value_0[89] = *(u8*)(hdr_6 + 106);
      vector_register_1073956208_value_0[90] = *(u8*)(hdr_6 + 107);
      vector_register_1073956208_value_0[91] = *(u8*)(hdr_6 + 108);
      vector_register_1073956208_value_0[92] = *(u8*)(hdr_6 + 109);
      vector_register_1073956208_value_0[93] = *(u8*)(hdr_6 + 110);
      vector_register_1073956208_value_0[94] = *(u8*)(hdr_6 + 111);
      vector_register_1073956208_value_0[95] = *(u8*)(hdr_6 + 112);
      vector_register_1073956208_value_0[96] = *(u8*)(hdr_6 + 113);
      vector_register_1073956208_value_0[97] = *(u8*)(hdr_6 + 114);
      vector_register_1073956208_value_0[98] = *(u8*)(hdr_6 + 115);
      vector_register_1073956208_value_0[99] = *(u8*)(hdr_6 + 116);
      vector_register_1073956208_value_0[100] = *(u8*)(hdr_6 + 117);
      vector_register_1073956208_value_0[101] = *(u8*)(hdr_6 + 118);
      vector_register_1073956208_value_0[102] = *(u8*)(hdr_6 + 119);
      vector_register_1073956208_value_0[103] = *(u8*)(hdr_6 + 120);
      vector_register_1073956208_value_0[104] = *(u8*)(hdr_6 + 121);
      vector_register_1073956208_value_0[105] = *(u8*)(hdr_6 + 122);
      vector_register_1073956208_value_0[106] = *(u8*)(hdr_6 + 123);
      vector_register_1073956208_value_0[107] = *(u8*)(hdr_6 + 124);
      vector_register_1073956208_value_0[108] = *(u8*)(hdr_6 + 125);
      vector_register_1073956208_value_0[109] = *(u8*)(hdr_6 + 126);
      vector_register_1073956208_value_0[110] = *(u8*)(hdr_6 + 127);
      vector_register_1073956208_value_0[111] = *(u8*)(hdr_6 + 128);
      vector_register_1073956208_value_0[112] = *(u8*)(hdr_6 + 129);
      vector_register_1073956208_value_0[113] = *(u8*)(hdr_6 + 130);
      vector_register_1073956208_value_0[114] = *(u8*)(hdr_6 + 131);
      vector_register_1073956208_value_0[115] = *(u8*)(hdr_6 + 132);
      vector_register_1073956208_value_0[116] = *(u8*)(hdr_6 + 133);
      vector_register_1073956208_value_0[117] = *(u8*)(hdr_6 + 134);
      vector_register_1073956208_value_0[118] = *(u8*)(hdr_6 + 135);
      vector_register_1073956208_value_0[119] = *(u8*)(hdr_6 + 136);
      vector_register_1073956208_value_0[120] = *(u8*)(hdr_6 + 137);
      vector_register_1073956208_value_0[121] = *(u8*)(hdr_6 + 138);
      vector_register_1073956208_value_0[122] = *(u8*)(hdr_6 + 139);
      vector_register_1073956208_value_0[123] = *(u8*)(hdr_6 + 140);
      vector_register_1073956208_value_0[124] = *(u8*)(hdr_6 + 141);
      vector_register_1073956208_value_0[125] = *(u8*)(hdr_6 + 142);
      vector_register_1073956208_value_0[126] = *(u8*)(hdr_6 + 143);
      vector_register_1073956208_value_0[127] = *(u8*)(hdr_6 + 144);
      state->vector_register_1073956208.put(allocated_index_0, vector_register_1073956208_value_0);
      // EP node  10196
      // BDD node 28:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760256)[(Concat w1184 (Read w8 (w32 915) packet_chunks) (Concat w1176 (Read w8 (w32 914) packet_chunks) (Concat w1168 (w8 0) (ReadLSB w1160 (w32 768) packet_chunks))))])
      hdr_6[145] = 0;
      // EP node  10265
      // BDD node 29:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073760000)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
      hdr_5[0] = *(u8*)(hdr_5 + 2);
      hdr_5[1] = *(u8*)(hdr_5 + 3);
      hdr_5[2] = *(u8*)(hdr_5 + 0);
      hdr_5[3] = *(u8*)(hdr_5 + 1);
      // EP node  10335
      // BDD node 30:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759744)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
      hdr_4[12] = *(u8*)(hdr_4 + 16);
      hdr_4[13] = *(u8*)(hdr_4 + 17);
      hdr_4[14] = *(u8*)(hdr_4 + 18);
      hdr_4[15] = *(u8*)(hdr_4 + 19);
      hdr_4[16] = *(u8*)(hdr_4 + 12);
      hdr_4[17] = *(u8*)(hdr_4 + 13);
      hdr_4[18] = *(u8*)(hdr_4 + 14);
      hdr_4[19] = *(u8*)(hdr_4 + 15);
      // EP node  10406
      // BDD node 31:packet_return_chunk(p:(w64 1074049376), the_chunk:(w64 1073759488)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
      hdr_0[0] = *(u8*)(hdr_0 + 6);
      hdr_0[1] = *(u8*)(hdr_0 + 7);
      hdr_0[2] = *(u8*)(hdr_0 + 8);
      hdr_0[3] = *(u8*)(hdr_0 + 9);
      hdr_0[4] = *(u8*)(hdr_0 + 10);
      hdr_0[5] = *(u8*)(hdr_0 + 11);
      hdr_0[6] = *(u8*)(hdr_0 + 0);
      hdr_0[7] = *(u8*)(hdr_0 + 1);
      hdr_0[8] = *(u8*)(hdr_0 + 2);
      hdr_0[9] = *(u8*)(hdr_0 + 3);
      hdr_0[10] = *(u8*)(hdr_0 + 4);
      hdr_0[11] = *(u8*)(hdr_0 + 5);
      // EP node  10478
      // BDD node 32:FORWARD
      cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
    }
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return forward;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
