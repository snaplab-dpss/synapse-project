#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  MapTable map_table_1074053168;
  VectorTable vector_table_1074066992;
  DchainTable dchain_table_1074085152;
  VectorTable vector_table_1074085576;
  VectorTable vector_table_1074102792;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      map_table_1074053168("map_table_1074053168",{"Ingress.map_table_1074053168_167",}, 1000LL),
      vector_table_1074066992("vector_table_1074066992",{"Ingress.vector_table_1074066992_144",}),
      dchain_table_1074085152("dchain_table_1074085152",{"Ingress.dchain_table_1074085152_189","Ingress.dchain_table_1074085152_142","Ingress.dchain_table_1074085152_146","Ingress.dchain_table_1074085152_273",}, 1000LL),
      vector_table_1074085576("vector_table_1074085576",{"Ingress.vector_table_1074085576_139",}),
      vector_table_1074102792("vector_table_1074102792",{"Ingress.vector_table_1074102792_191","Ingress.vector_table_1074102792_149",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 12), map_out:(w64 1074052904)[(w64 0) -> (w64 1074053168)])
  // Module DataplaneMapTableAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 12), capacity:(w32 65536), vector_out:(w64 1074052912)[(w64 0) -> (w64 1074066992)])
  // Module DataplaneVectorTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074052920)[ -> (w64 1074085152)])
  // Module DataplaneDchainTableAllocate
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074052928)[(w64 0) -> (w64 1074085576)])
  // Module DataplaneVectorTableAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074052936)[(w64 0) -> (w64 1074102792)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_borrow(vector:(w64 1074085576), index:(w32 0), val_out:(w64 1074052776)[ -> (w64 1074099472)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074085576), index:(w32 0), value:(w64 1074099472)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_0(4);
  vector_table_1074085576_value_0[0] = 0;
  vector_table_1074085576_value_0[1] = 0;
  vector_table_1074085576_value_0[2] = 0;
  vector_table_1074085576_value_0[3] = 1;
  state->vector_table_1074085576.write(0, vector_table_1074085576_value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074102792), index:(w32 0), val_out:(w64 1074052840)[ -> (w64 1074116688)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074102792), index:(w32 0), value:(w64 1074116688)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_0(2);
  vector_table_1074102792_value_0[0] = 0;
  vector_table_1074102792_value_0[1] = 1;
  state->vector_table_1074102792.write(0, vector_table_1074102792_value_0);
  // BDD node 9:vector_borrow(vector:(w64 1074085576), index:(w32 1), val_out:(w64 1074052776)[ -> (w64 1074099496)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074085576), index:(w32 1), value:(w64 1074099496)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_1(4);
  vector_table_1074085576_value_1[0] = 0;
  vector_table_1074085576_value_1[1] = 0;
  vector_table_1074085576_value_1[2] = 0;
  vector_table_1074085576_value_1[3] = 0;
  state->vector_table_1074085576.write(1, vector_table_1074085576_value_1);
  // BDD node 11:vector_borrow(vector:(w64 1074102792), index:(w32 1), val_out:(w64 1074052840)[ -> (w64 1074116712)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074102792), index:(w32 1), value:(w64 1074116712)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_1(2);
  vector_table_1074102792_value_1[0] = 0;
  vector_table_1074102792_value_1[1] = 0;
  state->vector_table_1074102792.write(1, vector_table_1074102792_value_1);
  // BDD node 13:vector_borrow(vector:(w64 1074085576), index:(w32 2), val_out:(w64 1074052776)[ -> (w64 1074099520)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074085576), index:(w32 2), value:(w64 1074099520)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_2(4);
  vector_table_1074085576_value_2[0] = 0;
  vector_table_1074085576_value_2[1] = 0;
  vector_table_1074085576_value_2[2] = 0;
  vector_table_1074085576_value_2[3] = 1;
  state->vector_table_1074085576.write(2, vector_table_1074085576_value_2);
  // BDD node 15:vector_borrow(vector:(w64 1074102792), index:(w32 2), val_out:(w64 1074052840)[ -> (w64 1074116736)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074102792), index:(w32 2), value:(w64 1074116736)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_2(2);
  vector_table_1074102792_value_2[0] = 0;
  vector_table_1074102792_value_2[1] = 3;
  state->vector_table_1074102792.write(2, vector_table_1074102792_value_2);
  // BDD node 17:vector_borrow(vector:(w64 1074085576), index:(w32 3), val_out:(w64 1074052776)[ -> (w64 1074099544)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074085576), index:(w32 3), value:(w64 1074099544)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_3(4);
  vector_table_1074085576_value_3[0] = 0;
  vector_table_1074085576_value_3[1] = 0;
  vector_table_1074085576_value_3[2] = 0;
  vector_table_1074085576_value_3[3] = 0;
  state->vector_table_1074085576.write(3, vector_table_1074085576_value_3);
  // BDD node 19:vector_borrow(vector:(w64 1074102792), index:(w32 3), val_out:(w64 1074052840)[ -> (w64 1074116760)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074102792), index:(w32 3), value:(w64 1074116760)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_3(2);
  vector_table_1074102792_value_3[0] = 0;
  vector_table_1074102792_value_3[1] = 2;
  state->vector_table_1074102792.write(3, vector_table_1074102792_value_3);
  // BDD node 21:vector_borrow(vector:(w64 1074085576), index:(w32 4), val_out:(w64 1074052776)[ -> (w64 1074099568)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074085576), index:(w32 4), value:(w64 1074099568)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_4(4);
  vector_table_1074085576_value_4[0] = 0;
  vector_table_1074085576_value_4[1] = 0;
  vector_table_1074085576_value_4[2] = 0;
  vector_table_1074085576_value_4[3] = 1;
  state->vector_table_1074085576.write(4, vector_table_1074085576_value_4);
  // BDD node 23:vector_borrow(vector:(w64 1074102792), index:(w32 4), val_out:(w64 1074052840)[ -> (w64 1074116784)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074102792), index:(w32 4), value:(w64 1074116784)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_4(2);
  vector_table_1074102792_value_4[0] = 0;
  vector_table_1074102792_value_4[1] = 5;
  state->vector_table_1074102792.write(4, vector_table_1074102792_value_4);
  // BDD node 25:vector_borrow(vector:(w64 1074085576), index:(w32 5), val_out:(w64 1074052776)[ -> (w64 1074099592)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074085576), index:(w32 5), value:(w64 1074099592)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_5(4);
  vector_table_1074085576_value_5[0] = 0;
  vector_table_1074085576_value_5[1] = 0;
  vector_table_1074085576_value_5[2] = 0;
  vector_table_1074085576_value_5[3] = 0;
  state->vector_table_1074085576.write(5, vector_table_1074085576_value_5);
  // BDD node 27:vector_borrow(vector:(w64 1074102792), index:(w32 5), val_out:(w64 1074052840)[ -> (w64 1074116808)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074102792), index:(w32 5), value:(w64 1074116808)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_5(2);
  vector_table_1074102792_value_5[0] = 0;
  vector_table_1074102792_value_5[1] = 4;
  state->vector_table_1074102792.write(5, vector_table_1074102792_value_5);
  // BDD node 29:vector_borrow(vector:(w64 1074085576), index:(w32 6), val_out:(w64 1074052776)[ -> (w64 1074099616)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074085576), index:(w32 6), value:(w64 1074099616)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_6(4);
  vector_table_1074085576_value_6[0] = 0;
  vector_table_1074085576_value_6[1] = 0;
  vector_table_1074085576_value_6[2] = 0;
  vector_table_1074085576_value_6[3] = 1;
  state->vector_table_1074085576.write(6, vector_table_1074085576_value_6);
  // BDD node 31:vector_borrow(vector:(w64 1074102792), index:(w32 6), val_out:(w64 1074052840)[ -> (w64 1074116832)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074102792), index:(w32 6), value:(w64 1074116832)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_6(2);
  vector_table_1074102792_value_6[0] = 0;
  vector_table_1074102792_value_6[1] = 7;
  state->vector_table_1074102792.write(6, vector_table_1074102792_value_6);
  // BDD node 33:vector_borrow(vector:(w64 1074085576), index:(w32 7), val_out:(w64 1074052776)[ -> (w64 1074099640)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074085576), index:(w32 7), value:(w64 1074099640)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_7(4);
  vector_table_1074085576_value_7[0] = 0;
  vector_table_1074085576_value_7[1] = 0;
  vector_table_1074085576_value_7[2] = 0;
  vector_table_1074085576_value_7[3] = 0;
  state->vector_table_1074085576.write(7, vector_table_1074085576_value_7);
  // BDD node 35:vector_borrow(vector:(w64 1074102792), index:(w32 7), val_out:(w64 1074052840)[ -> (w64 1074116856)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074102792), index:(w32 7), value:(w64 1074116856)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_7(2);
  vector_table_1074102792_value_7[0] = 0;
  vector_table_1074102792_value_7[1] = 6;
  state->vector_table_1074102792.write(7, vector_table_1074102792_value_7);
  // BDD node 37:vector_borrow(vector:(w64 1074085576), index:(w32 8), val_out:(w64 1074052776)[ -> (w64 1074099664)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074085576), index:(w32 8), value:(w64 1074099664)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_8(4);
  vector_table_1074085576_value_8[0] = 0;
  vector_table_1074085576_value_8[1] = 0;
  vector_table_1074085576_value_8[2] = 0;
  vector_table_1074085576_value_8[3] = 1;
  state->vector_table_1074085576.write(8, vector_table_1074085576_value_8);
  // BDD node 39:vector_borrow(vector:(w64 1074102792), index:(w32 8), val_out:(w64 1074052840)[ -> (w64 1074116880)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074102792), index:(w32 8), value:(w64 1074116880)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_8(2);
  vector_table_1074102792_value_8[0] = 0;
  vector_table_1074102792_value_8[1] = 9;
  state->vector_table_1074102792.write(8, vector_table_1074102792_value_8);
  // BDD node 41:vector_borrow(vector:(w64 1074085576), index:(w32 9), val_out:(w64 1074052776)[ -> (w64 1074099688)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074085576), index:(w32 9), value:(w64 1074099688)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_9(4);
  vector_table_1074085576_value_9[0] = 0;
  vector_table_1074085576_value_9[1] = 0;
  vector_table_1074085576_value_9[2] = 0;
  vector_table_1074085576_value_9[3] = 0;
  state->vector_table_1074085576.write(9, vector_table_1074085576_value_9);
  // BDD node 43:vector_borrow(vector:(w64 1074102792), index:(w32 9), val_out:(w64 1074052840)[ -> (w64 1074116904)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074102792), index:(w32 9), value:(w64 1074116904)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_9(2);
  vector_table_1074102792_value_9[0] = 0;
  vector_table_1074102792_value_9[1] = 8;
  state->vector_table_1074102792.write(9, vector_table_1074102792_value_9);
  // BDD node 45:vector_borrow(vector:(w64 1074085576), index:(w32 10), val_out:(w64 1074052776)[ -> (w64 1074099712)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074085576), index:(w32 10), value:(w64 1074099712)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_10(4);
  vector_table_1074085576_value_10[0] = 0;
  vector_table_1074085576_value_10[1] = 0;
  vector_table_1074085576_value_10[2] = 0;
  vector_table_1074085576_value_10[3] = 1;
  state->vector_table_1074085576.write(10, vector_table_1074085576_value_10);
  // BDD node 47:vector_borrow(vector:(w64 1074102792), index:(w32 10), val_out:(w64 1074052840)[ -> (w64 1074116928)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074102792), index:(w32 10), value:(w64 1074116928)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_10(2);
  vector_table_1074102792_value_10[0] = 0;
  vector_table_1074102792_value_10[1] = 11;
  state->vector_table_1074102792.write(10, vector_table_1074102792_value_10);
  // BDD node 49:vector_borrow(vector:(w64 1074085576), index:(w32 11), val_out:(w64 1074052776)[ -> (w64 1074099736)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074085576), index:(w32 11), value:(w64 1074099736)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_11(4);
  vector_table_1074085576_value_11[0] = 0;
  vector_table_1074085576_value_11[1] = 0;
  vector_table_1074085576_value_11[2] = 0;
  vector_table_1074085576_value_11[3] = 0;
  state->vector_table_1074085576.write(11, vector_table_1074085576_value_11);
  // BDD node 51:vector_borrow(vector:(w64 1074102792), index:(w32 11), val_out:(w64 1074052840)[ -> (w64 1074116952)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074102792), index:(w32 11), value:(w64 1074116952)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_11(2);
  vector_table_1074102792_value_11[0] = 0;
  vector_table_1074102792_value_11[1] = 10;
  state->vector_table_1074102792.write(11, vector_table_1074102792_value_11);
  // BDD node 53:vector_borrow(vector:(w64 1074085576), index:(w32 12), val_out:(w64 1074052776)[ -> (w64 1074099760)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074085576), index:(w32 12), value:(w64 1074099760)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_12(4);
  vector_table_1074085576_value_12[0] = 0;
  vector_table_1074085576_value_12[1] = 0;
  vector_table_1074085576_value_12[2] = 0;
  vector_table_1074085576_value_12[3] = 1;
  state->vector_table_1074085576.write(12, vector_table_1074085576_value_12);
  // BDD node 55:vector_borrow(vector:(w64 1074102792), index:(w32 12), val_out:(w64 1074052840)[ -> (w64 1074116976)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074102792), index:(w32 12), value:(w64 1074116976)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_12(2);
  vector_table_1074102792_value_12[0] = 0;
  vector_table_1074102792_value_12[1] = 13;
  state->vector_table_1074102792.write(12, vector_table_1074102792_value_12);
  // BDD node 57:vector_borrow(vector:(w64 1074085576), index:(w32 13), val_out:(w64 1074052776)[ -> (w64 1074099784)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074085576), index:(w32 13), value:(w64 1074099784)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_13(4);
  vector_table_1074085576_value_13[0] = 0;
  vector_table_1074085576_value_13[1] = 0;
  vector_table_1074085576_value_13[2] = 0;
  vector_table_1074085576_value_13[3] = 0;
  state->vector_table_1074085576.write(13, vector_table_1074085576_value_13);
  // BDD node 59:vector_borrow(vector:(w64 1074102792), index:(w32 13), val_out:(w64 1074052840)[ -> (w64 1074117000)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074102792), index:(w32 13), value:(w64 1074117000)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_13(2);
  vector_table_1074102792_value_13[0] = 0;
  vector_table_1074102792_value_13[1] = 12;
  state->vector_table_1074102792.write(13, vector_table_1074102792_value_13);
  // BDD node 61:vector_borrow(vector:(w64 1074085576), index:(w32 14), val_out:(w64 1074052776)[ -> (w64 1074099808)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074085576), index:(w32 14), value:(w64 1074099808)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_14(4);
  vector_table_1074085576_value_14[0] = 0;
  vector_table_1074085576_value_14[1] = 0;
  vector_table_1074085576_value_14[2] = 0;
  vector_table_1074085576_value_14[3] = 1;
  state->vector_table_1074085576.write(14, vector_table_1074085576_value_14);
  // BDD node 63:vector_borrow(vector:(w64 1074102792), index:(w32 14), val_out:(w64 1074052840)[ -> (w64 1074117024)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074102792), index:(w32 14), value:(w64 1074117024)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_14(2);
  vector_table_1074102792_value_14[0] = 0;
  vector_table_1074102792_value_14[1] = 15;
  state->vector_table_1074102792.write(14, vector_table_1074102792_value_14);
  // BDD node 65:vector_borrow(vector:(w64 1074085576), index:(w32 15), val_out:(w64 1074052776)[ -> (w64 1074099832)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074085576), index:(w32 15), value:(w64 1074099832)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_15(4);
  vector_table_1074085576_value_15[0] = 0;
  vector_table_1074085576_value_15[1] = 0;
  vector_table_1074085576_value_15[2] = 0;
  vector_table_1074085576_value_15[3] = 0;
  state->vector_table_1074085576.write(15, vector_table_1074085576_value_15);
  // BDD node 67:vector_borrow(vector:(w64 1074102792), index:(w32 15), val_out:(w64 1074052840)[ -> (w64 1074117048)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074102792), index:(w32 15), value:(w64 1074117048)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_15(2);
  vector_table_1074102792_value_15[0] = 0;
  vector_table_1074102792_value_15[1] = 14;
  state->vector_table_1074102792.write(15, vector_table_1074102792_value_15);
  // BDD node 69:vector_borrow(vector:(w64 1074085576), index:(w32 16), val_out:(w64 1074052776)[ -> (w64 1074099856)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074085576), index:(w32 16), value:(w64 1074099856)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_16(4);
  vector_table_1074085576_value_16[0] = 0;
  vector_table_1074085576_value_16[1] = 0;
  vector_table_1074085576_value_16[2] = 0;
  vector_table_1074085576_value_16[3] = 1;
  state->vector_table_1074085576.write(16, vector_table_1074085576_value_16);
  // BDD node 71:vector_borrow(vector:(w64 1074102792), index:(w32 16), val_out:(w64 1074052840)[ -> (w64 1074117072)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074102792), index:(w32 16), value:(w64 1074117072)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_16(2);
  vector_table_1074102792_value_16[0] = 0;
  vector_table_1074102792_value_16[1] = 17;
  state->vector_table_1074102792.write(16, vector_table_1074102792_value_16);
  // BDD node 73:vector_borrow(vector:(w64 1074085576), index:(w32 17), val_out:(w64 1074052776)[ -> (w64 1074099880)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074085576), index:(w32 17), value:(w64 1074099880)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_17(4);
  vector_table_1074085576_value_17[0] = 0;
  vector_table_1074085576_value_17[1] = 0;
  vector_table_1074085576_value_17[2] = 0;
  vector_table_1074085576_value_17[3] = 0;
  state->vector_table_1074085576.write(17, vector_table_1074085576_value_17);
  // BDD node 75:vector_borrow(vector:(w64 1074102792), index:(w32 17), val_out:(w64 1074052840)[ -> (w64 1074117096)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074102792), index:(w32 17), value:(w64 1074117096)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_17(2);
  vector_table_1074102792_value_17[0] = 0;
  vector_table_1074102792_value_17[1] = 16;
  state->vector_table_1074102792.write(17, vector_table_1074102792_value_17);
  // BDD node 77:vector_borrow(vector:(w64 1074085576), index:(w32 18), val_out:(w64 1074052776)[ -> (w64 1074099904)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074085576), index:(w32 18), value:(w64 1074099904)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_18(4);
  vector_table_1074085576_value_18[0] = 0;
  vector_table_1074085576_value_18[1] = 0;
  vector_table_1074085576_value_18[2] = 0;
  vector_table_1074085576_value_18[3] = 1;
  state->vector_table_1074085576.write(18, vector_table_1074085576_value_18);
  // BDD node 79:vector_borrow(vector:(w64 1074102792), index:(w32 18), val_out:(w64 1074052840)[ -> (w64 1074117120)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074102792), index:(w32 18), value:(w64 1074117120)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_18(2);
  vector_table_1074102792_value_18[0] = 0;
  vector_table_1074102792_value_18[1] = 19;
  state->vector_table_1074102792.write(18, vector_table_1074102792_value_18);
  // BDD node 81:vector_borrow(vector:(w64 1074085576), index:(w32 19), val_out:(w64 1074052776)[ -> (w64 1074099928)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074085576), index:(w32 19), value:(w64 1074099928)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_19(4);
  vector_table_1074085576_value_19[0] = 0;
  vector_table_1074085576_value_19[1] = 0;
  vector_table_1074085576_value_19[2] = 0;
  vector_table_1074085576_value_19[3] = 0;
  state->vector_table_1074085576.write(19, vector_table_1074085576_value_19);
  // BDD node 83:vector_borrow(vector:(w64 1074102792), index:(w32 19), val_out:(w64 1074052840)[ -> (w64 1074117144)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074102792), index:(w32 19), value:(w64 1074117144)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_19(2);
  vector_table_1074102792_value_19[0] = 0;
  vector_table_1074102792_value_19[1] = 18;
  state->vector_table_1074102792.write(19, vector_table_1074102792_value_19);
  // BDD node 85:vector_borrow(vector:(w64 1074085576), index:(w32 20), val_out:(w64 1074052776)[ -> (w64 1074099952)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074085576), index:(w32 20), value:(w64 1074099952)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_20(4);
  vector_table_1074085576_value_20[0] = 0;
  vector_table_1074085576_value_20[1] = 0;
  vector_table_1074085576_value_20[2] = 0;
  vector_table_1074085576_value_20[3] = 1;
  state->vector_table_1074085576.write(20, vector_table_1074085576_value_20);
  // BDD node 87:vector_borrow(vector:(w64 1074102792), index:(w32 20), val_out:(w64 1074052840)[ -> (w64 1074117168)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074102792), index:(w32 20), value:(w64 1074117168)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_20(2);
  vector_table_1074102792_value_20[0] = 0;
  vector_table_1074102792_value_20[1] = 21;
  state->vector_table_1074102792.write(20, vector_table_1074102792_value_20);
  // BDD node 89:vector_borrow(vector:(w64 1074085576), index:(w32 21), val_out:(w64 1074052776)[ -> (w64 1074099976)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074085576), index:(w32 21), value:(w64 1074099976)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_21(4);
  vector_table_1074085576_value_21[0] = 0;
  vector_table_1074085576_value_21[1] = 0;
  vector_table_1074085576_value_21[2] = 0;
  vector_table_1074085576_value_21[3] = 0;
  state->vector_table_1074085576.write(21, vector_table_1074085576_value_21);
  // BDD node 91:vector_borrow(vector:(w64 1074102792), index:(w32 21), val_out:(w64 1074052840)[ -> (w64 1074117192)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074102792), index:(w32 21), value:(w64 1074117192)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_21(2);
  vector_table_1074102792_value_21[0] = 0;
  vector_table_1074102792_value_21[1] = 20;
  state->vector_table_1074102792.write(21, vector_table_1074102792_value_21);
  // BDD node 93:vector_borrow(vector:(w64 1074085576), index:(w32 22), val_out:(w64 1074052776)[ -> (w64 1074100000)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074085576), index:(w32 22), value:(w64 1074100000)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_22(4);
  vector_table_1074085576_value_22[0] = 0;
  vector_table_1074085576_value_22[1] = 0;
  vector_table_1074085576_value_22[2] = 0;
  vector_table_1074085576_value_22[3] = 1;
  state->vector_table_1074085576.write(22, vector_table_1074085576_value_22);
  // BDD node 95:vector_borrow(vector:(w64 1074102792), index:(w32 22), val_out:(w64 1074052840)[ -> (w64 1074117216)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074102792), index:(w32 22), value:(w64 1074117216)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_22(2);
  vector_table_1074102792_value_22[0] = 0;
  vector_table_1074102792_value_22[1] = 23;
  state->vector_table_1074102792.write(22, vector_table_1074102792_value_22);
  // BDD node 97:vector_borrow(vector:(w64 1074085576), index:(w32 23), val_out:(w64 1074052776)[ -> (w64 1074100024)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074085576), index:(w32 23), value:(w64 1074100024)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_23(4);
  vector_table_1074085576_value_23[0] = 0;
  vector_table_1074085576_value_23[1] = 0;
  vector_table_1074085576_value_23[2] = 0;
  vector_table_1074085576_value_23[3] = 0;
  state->vector_table_1074085576.write(23, vector_table_1074085576_value_23);
  // BDD node 99:vector_borrow(vector:(w64 1074102792), index:(w32 23), val_out:(w64 1074052840)[ -> (w64 1074117240)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074102792), index:(w32 23), value:(w64 1074117240)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_23(2);
  vector_table_1074102792_value_23[0] = 0;
  vector_table_1074102792_value_23[1] = 22;
  state->vector_table_1074102792.write(23, vector_table_1074102792_value_23);
  // BDD node 101:vector_borrow(vector:(w64 1074085576), index:(w32 24), val_out:(w64 1074052776)[ -> (w64 1074100048)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074085576), index:(w32 24), value:(w64 1074100048)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_24(4);
  vector_table_1074085576_value_24[0] = 0;
  vector_table_1074085576_value_24[1] = 0;
  vector_table_1074085576_value_24[2] = 0;
  vector_table_1074085576_value_24[3] = 1;
  state->vector_table_1074085576.write(24, vector_table_1074085576_value_24);
  // BDD node 103:vector_borrow(vector:(w64 1074102792), index:(w32 24), val_out:(w64 1074052840)[ -> (w64 1074117264)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074102792), index:(w32 24), value:(w64 1074117264)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_24(2);
  vector_table_1074102792_value_24[0] = 0;
  vector_table_1074102792_value_24[1] = 25;
  state->vector_table_1074102792.write(24, vector_table_1074102792_value_24);
  // BDD node 105:vector_borrow(vector:(w64 1074085576), index:(w32 25), val_out:(w64 1074052776)[ -> (w64 1074100072)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074085576), index:(w32 25), value:(w64 1074100072)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_25(4);
  vector_table_1074085576_value_25[0] = 0;
  vector_table_1074085576_value_25[1] = 0;
  vector_table_1074085576_value_25[2] = 0;
  vector_table_1074085576_value_25[3] = 0;
  state->vector_table_1074085576.write(25, vector_table_1074085576_value_25);
  // BDD node 107:vector_borrow(vector:(w64 1074102792), index:(w32 25), val_out:(w64 1074052840)[ -> (w64 1074117288)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074102792), index:(w32 25), value:(w64 1074117288)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_25(2);
  vector_table_1074102792_value_25[0] = 0;
  vector_table_1074102792_value_25[1] = 24;
  state->vector_table_1074102792.write(25, vector_table_1074102792_value_25);
  // BDD node 109:vector_borrow(vector:(w64 1074085576), index:(w32 26), val_out:(w64 1074052776)[ -> (w64 1074100096)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074085576), index:(w32 26), value:(w64 1074100096)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_26(4);
  vector_table_1074085576_value_26[0] = 0;
  vector_table_1074085576_value_26[1] = 0;
  vector_table_1074085576_value_26[2] = 0;
  vector_table_1074085576_value_26[3] = 1;
  state->vector_table_1074085576.write(26, vector_table_1074085576_value_26);
  // BDD node 111:vector_borrow(vector:(w64 1074102792), index:(w32 26), val_out:(w64 1074052840)[ -> (w64 1074117312)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074102792), index:(w32 26), value:(w64 1074117312)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_26(2);
  vector_table_1074102792_value_26[0] = 0;
  vector_table_1074102792_value_26[1] = 27;
  state->vector_table_1074102792.write(26, vector_table_1074102792_value_26);
  // BDD node 113:vector_borrow(vector:(w64 1074085576), index:(w32 27), val_out:(w64 1074052776)[ -> (w64 1074100120)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074085576), index:(w32 27), value:(w64 1074100120)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_27(4);
  vector_table_1074085576_value_27[0] = 0;
  vector_table_1074085576_value_27[1] = 0;
  vector_table_1074085576_value_27[2] = 0;
  vector_table_1074085576_value_27[3] = 0;
  state->vector_table_1074085576.write(27, vector_table_1074085576_value_27);
  // BDD node 115:vector_borrow(vector:(w64 1074102792), index:(w32 27), val_out:(w64 1074052840)[ -> (w64 1074117336)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074102792), index:(w32 27), value:(w64 1074117336)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_27(2);
  vector_table_1074102792_value_27[0] = 0;
  vector_table_1074102792_value_27[1] = 26;
  state->vector_table_1074102792.write(27, vector_table_1074102792_value_27);
  // BDD node 117:vector_borrow(vector:(w64 1074085576), index:(w32 28), val_out:(w64 1074052776)[ -> (w64 1074100144)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074085576), index:(w32 28), value:(w64 1074100144)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_28(4);
  vector_table_1074085576_value_28[0] = 0;
  vector_table_1074085576_value_28[1] = 0;
  vector_table_1074085576_value_28[2] = 0;
  vector_table_1074085576_value_28[3] = 1;
  state->vector_table_1074085576.write(28, vector_table_1074085576_value_28);
  // BDD node 119:vector_borrow(vector:(w64 1074102792), index:(w32 28), val_out:(w64 1074052840)[ -> (w64 1074117360)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074102792), index:(w32 28), value:(w64 1074117360)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_28(2);
  vector_table_1074102792_value_28[0] = 0;
  vector_table_1074102792_value_28[1] = 29;
  state->vector_table_1074102792.write(28, vector_table_1074102792_value_28);
  // BDD node 121:vector_borrow(vector:(w64 1074085576), index:(w32 29), val_out:(w64 1074052776)[ -> (w64 1074100168)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074085576), index:(w32 29), value:(w64 1074100168)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_29(4);
  vector_table_1074085576_value_29[0] = 0;
  vector_table_1074085576_value_29[1] = 0;
  vector_table_1074085576_value_29[2] = 0;
  vector_table_1074085576_value_29[3] = 0;
  state->vector_table_1074085576.write(29, vector_table_1074085576_value_29);
  // BDD node 123:vector_borrow(vector:(w64 1074102792), index:(w32 29), val_out:(w64 1074052840)[ -> (w64 1074117384)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074102792), index:(w32 29), value:(w64 1074117384)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_29(2);
  vector_table_1074102792_value_29[0] = 0;
  vector_table_1074102792_value_29[1] = 28;
  state->vector_table_1074102792.write(29, vector_table_1074102792_value_29);
  // BDD node 125:vector_borrow(vector:(w64 1074085576), index:(w32 30), val_out:(w64 1074052776)[ -> (w64 1074100192)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074085576), index:(w32 30), value:(w64 1074100192)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_30(4);
  vector_table_1074085576_value_30[0] = 0;
  vector_table_1074085576_value_30[1] = 0;
  vector_table_1074085576_value_30[2] = 0;
  vector_table_1074085576_value_30[3] = 1;
  state->vector_table_1074085576.write(30, vector_table_1074085576_value_30);
  // BDD node 127:vector_borrow(vector:(w64 1074102792), index:(w32 30), val_out:(w64 1074052840)[ -> (w64 1074117408)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074102792), index:(w32 30), value:(w64 1074117408)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_30(2);
  vector_table_1074102792_value_30[0] = 0;
  vector_table_1074102792_value_30[1] = 31;
  state->vector_table_1074102792.write(30, vector_table_1074102792_value_30);
  // BDD node 129:vector_borrow(vector:(w64 1074085576), index:(w32 31), val_out:(w64 1074052776)[ -> (w64 1074100216)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074085576), index:(w32 31), value:(w64 1074100216)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074085576_value_31(4);
  vector_table_1074085576_value_31[0] = 0;
  vector_table_1074085576_value_31[1] = 0;
  vector_table_1074085576_value_31[2] = 0;
  vector_table_1074085576_value_31[3] = 0;
  state->vector_table_1074085576.write(31, vector_table_1074085576_value_31);
  // BDD node 131:vector_borrow(vector:(w64 1074102792), index:(w32 31), val_out:(w64 1074052840)[ -> (w64 1074117432)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074102792), index:(w32 31), value:(w64 1074117432)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074102792_value_31(2);
  vector_table_1074102792_value_31[0] = 0;
  vector_table_1074102792_value_31[1] = 30;
  state->vector_table_1074102792.write(31, vector_table_1074102792_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 vector_data_r1;
  u32 map_has_this_key;
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

  if (bswap16(cpu_hdr->code_path) == 5705) {
    // EP node  5692
    // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  5693
    // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  5694
    // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  5695
    // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t value_0;
    state->vector_table_1074085576.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  5696
    // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  5697
      // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  5699
      // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    } else {
      // EP node  5698
      // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  5700
      // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      buffer_t map_table_1074053168_key_0(12);
      map_table_1074053168_key_0[0] = *(u8*)(hdr_1 + 12);
      map_table_1074053168_key_0[1] = *(u8*)(hdr_1 + 13);
      map_table_1074053168_key_0[2] = *(u8*)(hdr_1 + 14);
      map_table_1074053168_key_0[3] = *(u8*)(hdr_1 + 15);
      map_table_1074053168_key_0[4] = *(u8*)(hdr_1 + 16);
      map_table_1074053168_key_0[5] = *(u8*)(hdr_1 + 17);
      map_table_1074053168_key_0[6] = *(u8*)(hdr_1 + 18);
      map_table_1074053168_key_0[7] = *(u8*)(hdr_1 + 19);
      map_table_1074053168_key_0[8] = *(u8*)(hdr_2 + 0);
      map_table_1074053168_key_0[9] = *(u8*)(hdr_2 + 1);
      map_table_1074053168_key_0[10] = *(u8*)(hdr_2 + 2);
      map_table_1074053168_key_0[11] = *(u8*)(hdr_2 + 3);
      u32 value_1;
      bool found_0 = state->map_table_1074053168.get(map_table_1074053168_key_0, value_1);
      // EP node  5701
      // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_0)) {
        // EP node  5702
        // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  10135
        // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        u32 allocated_index_0;
        bool success_0 = state->dchain_table_1074085152.allocate_new_index(allocated_index_0);
        // EP node  10218
        // BDD node 170:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
        if ((0) == (success_0)) {
          // EP node  10219
          // BDD node 170:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
          // EP node  13174
          // BDD node 174:DROP
          result.forward = false;
        } else {
          // EP node  10220
          // BDD node 170:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
          // EP node  10476
          // BDD node 175:vector_borrow(vector:(w64 1074066992), index:(ReadLSB w32 (w32 0) new_index), val_out:(w64 1074228376)[ -> (w64 1074080888)])
          // EP node  10737
          // BDD node 176:map_put(map:(w64 1074053168), key:(w64 1074080888)[(Concat w96 (Read w8 (w32 515) packet_chunks) (Concat w88 (Read w8 (w32 514) packet_chunks) (Concat w80 (Read w8 (w32 513) packet_chunks) (Concat w72 (Read w8 (w32 512) packet_chunks) (ReadLSB w64 (w32 268) packet_chunks))))) -> (Concat w96 (Read w8 (w32 515) packet_chunks) (Concat w88 (Read w8 (w32 514) packet_chunks) (Concat w80 (Read w8 (w32 513) packet_chunks) (Concat w72 (Read w8 (w32 512) packet_chunks) (ReadLSB w64 (w32 268) packet_chunks)))))], value:(ReadLSB w32 (w32 0) new_index))
          buffer_t map_table_1074053168_key_1(12);
          map_table_1074053168_key_1[0] = *(u8*)(hdr_1 + 12);
          map_table_1074053168_key_1[1] = *(u8*)(hdr_1 + 13);
          map_table_1074053168_key_1[2] = *(u8*)(hdr_1 + 14);
          map_table_1074053168_key_1[3] = *(u8*)(hdr_1 + 15);
          map_table_1074053168_key_1[4] = *(u8*)(hdr_1 + 16);
          map_table_1074053168_key_1[5] = *(u8*)(hdr_1 + 17);
          map_table_1074053168_key_1[6] = *(u8*)(hdr_1 + 18);
          map_table_1074053168_key_1[7] = *(u8*)(hdr_1 + 19);
          map_table_1074053168_key_1[8] = *(u8*)(hdr_2 + 0);
          map_table_1074053168_key_1[9] = *(u8*)(hdr_2 + 1);
          map_table_1074053168_key_1[10] = *(u8*)(hdr_2 + 2);
          map_table_1074053168_key_1[11] = *(u8*)(hdr_2 + 3);
          state->map_table_1074053168.put(map_table_1074053168_key_1, allocated_index_0);
          // EP node  10913
          // BDD node 177:vector_return(vector:(w64 1074066992), index:(ReadLSB w32 (w32 0) new_index), value:(w64 1074080888)[(Concat w96 (Read w8 (w32 515) packet_chunks) (Concat w88 (Read w8 (w32 514) packet_chunks) (Concat w80 (Read w8 (w32 513) packet_chunks) (Concat w72 (Read w8 (w32 512) packet_chunks) (ReadLSB w64 (w32 268) packet_chunks)))))])
          buffer_t vector_table_1074066992_value_0(12);
          vector_table_1074066992_value_0[0] = *(u8*)(hdr_1 + 12);
          vector_table_1074066992_value_0[1] = *(u8*)(hdr_1 + 13);
          vector_table_1074066992_value_0[2] = *(u8*)(hdr_1 + 14);
          vector_table_1074066992_value_0[3] = *(u8*)(hdr_1 + 15);
          vector_table_1074066992_value_0[4] = *(u8*)(hdr_1 + 16);
          vector_table_1074066992_value_0[5] = *(u8*)(hdr_1 + 17);
          vector_table_1074066992_value_0[6] = *(u8*)(hdr_1 + 18);
          vector_table_1074066992_value_0[7] = *(u8*)(hdr_1 + 19);
          vector_table_1074066992_value_0[8] = *(u8*)(hdr_2 + 0);
          vector_table_1074066992_value_0[9] = *(u8*)(hdr_2 + 1);
          vector_table_1074066992_value_0[10] = *(u8*)(hdr_2 + 2);
          vector_table_1074066992_value_0[11] = *(u8*)(hdr_2 + 3);
          state->vector_table_1074066992.write(allocated_index_0, vector_table_1074066992_value_0);
          // EP node  11180
          // BDD node 178:nf_set_rte_ipv4_udptcp_checksum(ip_header:(w64 1073763600), l4_header:(w64 1073763856), packet:(w64 1074155304))
          trigger_update_ipv4_tcpudp_checksums = true;
          l3_hdr = (void *)hdr_1;
          l4_hdr = (void *)hdr_2;
          // EP node  11181
          // BDD node 179:vector_borrow(vector:(w64 1074102792), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074230176)[ -> (w64 1074116688)])
          buffer_t value_2;
          state->vector_table_1074102792.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_2);
          // EP node  11363
          // BDD node 184:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
          if ((bswap32(cpu_hdr_extra->DEVICE) & 65535) != ((u16)value_2.get(0, 2))) {
            // EP node  11364
            // BDD node 184:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
            // EP node  11551
            // BDD node 180:vector_return(vector:(w64 1074102792), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074116688)[(ReadLSB w16 (w32 0) vector_data_640)])
            // EP node  11741
            // BDD node 185:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
            if ((65535) != ((u16)value_2.get(0, 2))) {
              // EP node  11742
              // BDD node 185:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  11937
              // BDD node 181:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763856)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) new_index)))])
              hdr_2[0] = allocated_index_0 & 255;
              hdr_2[1] = (allocated_index_0>>8) & 255;
              // EP node  12037
              // BDD node 182:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763600)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
              hdr_1[12] = 1;
              hdr_1[13] = 2;
              hdr_1[14] = 3;
              hdr_1[15] = 4;
              // EP node  12238
              // BDD node 186:FORWARD
              cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
            } else {
              // EP node  11743
              // BDD node 185:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  12544
              // BDD node 288:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763856)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) new_index)))])
              hdr_2[0] = allocated_index_0 & 255;
              hdr_2[1] = (allocated_index_0>>8) & 255;
              // EP node  12856
              // BDD node 289:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763600)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
              hdr_1[12] = 1;
              hdr_1[13] = 2;
              hdr_1[14] = 3;
              hdr_1[15] = 4;
              // EP node  13388
              // BDD node 187:DROP
              result.forward = false;
            }
          } else {
            // EP node  11365
            // BDD node 184:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data_640)))
            // EP node  12340
            // BDD node 284:vector_return(vector:(w64 1074102792), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074116688)[(ReadLSB w16 (w32 0) vector_data_640)])
            // EP node  12751
            // BDD node 285:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763856)[(Concat w32 (Read w8 (w32 515) packet_chunks) (Concat w24 (Read w8 (w32 514) packet_chunks) (ReadLSB w16 (w32 0) new_index)))])
            hdr_2[0] = allocated_index_0 & 255;
            hdr_2[1] = (allocated_index_0>>8) & 255;
            // EP node  13067
            // BDD node 286:packet_return_chunk(p:(w64 1074210520), the_chunk:(w64 1073763600)[(Concat w160 (Read w8 (w32 275) packet_chunks) (Concat w152 (Read w8 (w32 274) packet_chunks) (Concat w144 (Read w8 (w32 273) packet_chunks) (Concat w136 (Read w8 (w32 272) packet_chunks) (Concat w128 (w8 4) (Concat w120 (w8 3) (Concat w112 (w8 2) (Concat w104 (w8 1) (Concat w96 (Read w8 (w32 1) checksum) (Concat w88 (Read w8 (w32 0) checksum) (ReadLSB w80 (w32 256) packet_chunks)))))))))))])
            hdr_1[12] = 1;
            hdr_1[13] = 2;
            hdr_1[14] = 3;
            hdr_1[15] = 4;
            // EP node  13496
            // BDD node 188:DROP
            result.forward = false;
          }
        }
      } else {
        // EP node  5703
        // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  5704
        // BDD node 169:dchain_allocate_new_index(chain:(w64 1074085152), index_out:(w64 1074228352)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    }
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
