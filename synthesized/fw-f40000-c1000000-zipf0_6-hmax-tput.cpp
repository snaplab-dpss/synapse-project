#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  FCFSCachedSet fcfs_cs_1074044048;
  VectorTable vector_table_1074076456;
  VectorTable vector_table_1074093672;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      fcfs_cs_1074044048("fcfs_cs_1074044048",{"Ingress.fcfs_cs_1074044048_table_142", "Ingress.fcfs_cs_1074044048_table_159", }, 1000LL),
      vector_table_1074076456("vector_table_1074076456",{"Ingress.vector_table_1074076456_139",}),
      vector_table_1074093672("vector_table_1074093672",{"Ingress.vector_table_1074093672_149","Ingress.vector_table_1074093672_187","Ingress.vector_table_1074093672_176",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 13), map_out:(w64 1074043784)[(w64 0) -> (w64 1074044048)])
  // Module DataplaneFCFSCachedSetAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074043800)[ -> (w64 1074076032)])
  // Module Ignore
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074043808)[(w64 0) -> (w64 1074076456)])
  // Module DataplaneVectorTableAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074043816)[(w64 0) -> (w64 1074093672)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_borrow(vector:(w64 1074076456), index:(w32 0), val_out:(w64 1074043656)[ -> (w64 1074090352)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074076456), index:(w32 0), value:(w64 1074090352)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_0(4);
  vector_table_1074076456_value_0[0] = 0;
  vector_table_1074076456_value_0[1] = 0;
  vector_table_1074076456_value_0[2] = 0;
  vector_table_1074076456_value_0[3] = 1;
  state->vector_table_1074076456.write(0, vector_table_1074076456_value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074093672), index:(w32 0), val_out:(w64 1074043720)[ -> (w64 1074107568)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074093672), index:(w32 0), value:(w64 1074107568)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_0(2);
  vector_table_1074093672_value_0[0] = 0;
  vector_table_1074093672_value_0[1] = 1;
  state->vector_table_1074093672.write(0, vector_table_1074093672_value_0);
  // BDD node 9:vector_borrow(vector:(w64 1074076456), index:(w32 1), val_out:(w64 1074043656)[ -> (w64 1074090376)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074076456), index:(w32 1), value:(w64 1074090376)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_1(4);
  vector_table_1074076456_value_1[0] = 0;
  vector_table_1074076456_value_1[1] = 0;
  vector_table_1074076456_value_1[2] = 0;
  vector_table_1074076456_value_1[3] = 0;
  state->vector_table_1074076456.write(1, vector_table_1074076456_value_1);
  // BDD node 11:vector_borrow(vector:(w64 1074093672), index:(w32 1), val_out:(w64 1074043720)[ -> (w64 1074107592)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074093672), index:(w32 1), value:(w64 1074107592)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_1(2);
  vector_table_1074093672_value_1[0] = 0;
  vector_table_1074093672_value_1[1] = 0;
  state->vector_table_1074093672.write(1, vector_table_1074093672_value_1);
  // BDD node 13:vector_borrow(vector:(w64 1074076456), index:(w32 2), val_out:(w64 1074043656)[ -> (w64 1074090400)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074076456), index:(w32 2), value:(w64 1074090400)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_2(4);
  vector_table_1074076456_value_2[0] = 0;
  vector_table_1074076456_value_2[1] = 0;
  vector_table_1074076456_value_2[2] = 0;
  vector_table_1074076456_value_2[3] = 1;
  state->vector_table_1074076456.write(2, vector_table_1074076456_value_2);
  // BDD node 15:vector_borrow(vector:(w64 1074093672), index:(w32 2), val_out:(w64 1074043720)[ -> (w64 1074107616)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074093672), index:(w32 2), value:(w64 1074107616)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_2(2);
  vector_table_1074093672_value_2[0] = 0;
  vector_table_1074093672_value_2[1] = 3;
  state->vector_table_1074093672.write(2, vector_table_1074093672_value_2);
  // BDD node 17:vector_borrow(vector:(w64 1074076456), index:(w32 3), val_out:(w64 1074043656)[ -> (w64 1074090424)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074076456), index:(w32 3), value:(w64 1074090424)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_3(4);
  vector_table_1074076456_value_3[0] = 0;
  vector_table_1074076456_value_3[1] = 0;
  vector_table_1074076456_value_3[2] = 0;
  vector_table_1074076456_value_3[3] = 0;
  state->vector_table_1074076456.write(3, vector_table_1074076456_value_3);
  // BDD node 19:vector_borrow(vector:(w64 1074093672), index:(w32 3), val_out:(w64 1074043720)[ -> (w64 1074107640)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074093672), index:(w32 3), value:(w64 1074107640)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_3(2);
  vector_table_1074093672_value_3[0] = 0;
  vector_table_1074093672_value_3[1] = 2;
  state->vector_table_1074093672.write(3, vector_table_1074093672_value_3);
  // BDD node 21:vector_borrow(vector:(w64 1074076456), index:(w32 4), val_out:(w64 1074043656)[ -> (w64 1074090448)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074076456), index:(w32 4), value:(w64 1074090448)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_4(4);
  vector_table_1074076456_value_4[0] = 0;
  vector_table_1074076456_value_4[1] = 0;
  vector_table_1074076456_value_4[2] = 0;
  vector_table_1074076456_value_4[3] = 1;
  state->vector_table_1074076456.write(4, vector_table_1074076456_value_4);
  // BDD node 23:vector_borrow(vector:(w64 1074093672), index:(w32 4), val_out:(w64 1074043720)[ -> (w64 1074107664)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074093672), index:(w32 4), value:(w64 1074107664)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_4(2);
  vector_table_1074093672_value_4[0] = 0;
  vector_table_1074093672_value_4[1] = 5;
  state->vector_table_1074093672.write(4, vector_table_1074093672_value_4);
  // BDD node 25:vector_borrow(vector:(w64 1074076456), index:(w32 5), val_out:(w64 1074043656)[ -> (w64 1074090472)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074076456), index:(w32 5), value:(w64 1074090472)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_5(4);
  vector_table_1074076456_value_5[0] = 0;
  vector_table_1074076456_value_5[1] = 0;
  vector_table_1074076456_value_5[2] = 0;
  vector_table_1074076456_value_5[3] = 0;
  state->vector_table_1074076456.write(5, vector_table_1074076456_value_5);
  // BDD node 27:vector_borrow(vector:(w64 1074093672), index:(w32 5), val_out:(w64 1074043720)[ -> (w64 1074107688)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074093672), index:(w32 5), value:(w64 1074107688)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_5(2);
  vector_table_1074093672_value_5[0] = 0;
  vector_table_1074093672_value_5[1] = 4;
  state->vector_table_1074093672.write(5, vector_table_1074093672_value_5);
  // BDD node 29:vector_borrow(vector:(w64 1074076456), index:(w32 6), val_out:(w64 1074043656)[ -> (w64 1074090496)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074076456), index:(w32 6), value:(w64 1074090496)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_6(4);
  vector_table_1074076456_value_6[0] = 0;
  vector_table_1074076456_value_6[1] = 0;
  vector_table_1074076456_value_6[2] = 0;
  vector_table_1074076456_value_6[3] = 1;
  state->vector_table_1074076456.write(6, vector_table_1074076456_value_6);
  // BDD node 31:vector_borrow(vector:(w64 1074093672), index:(w32 6), val_out:(w64 1074043720)[ -> (w64 1074107712)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074093672), index:(w32 6), value:(w64 1074107712)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_6(2);
  vector_table_1074093672_value_6[0] = 0;
  vector_table_1074093672_value_6[1] = 7;
  state->vector_table_1074093672.write(6, vector_table_1074093672_value_6);
  // BDD node 33:vector_borrow(vector:(w64 1074076456), index:(w32 7), val_out:(w64 1074043656)[ -> (w64 1074090520)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074076456), index:(w32 7), value:(w64 1074090520)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_7(4);
  vector_table_1074076456_value_7[0] = 0;
  vector_table_1074076456_value_7[1] = 0;
  vector_table_1074076456_value_7[2] = 0;
  vector_table_1074076456_value_7[3] = 0;
  state->vector_table_1074076456.write(7, vector_table_1074076456_value_7);
  // BDD node 35:vector_borrow(vector:(w64 1074093672), index:(w32 7), val_out:(w64 1074043720)[ -> (w64 1074107736)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074093672), index:(w32 7), value:(w64 1074107736)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_7(2);
  vector_table_1074093672_value_7[0] = 0;
  vector_table_1074093672_value_7[1] = 6;
  state->vector_table_1074093672.write(7, vector_table_1074093672_value_7);
  // BDD node 37:vector_borrow(vector:(w64 1074076456), index:(w32 8), val_out:(w64 1074043656)[ -> (w64 1074090544)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074076456), index:(w32 8), value:(w64 1074090544)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_8(4);
  vector_table_1074076456_value_8[0] = 0;
  vector_table_1074076456_value_8[1] = 0;
  vector_table_1074076456_value_8[2] = 0;
  vector_table_1074076456_value_8[3] = 1;
  state->vector_table_1074076456.write(8, vector_table_1074076456_value_8);
  // BDD node 39:vector_borrow(vector:(w64 1074093672), index:(w32 8), val_out:(w64 1074043720)[ -> (w64 1074107760)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074093672), index:(w32 8), value:(w64 1074107760)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_8(2);
  vector_table_1074093672_value_8[0] = 0;
  vector_table_1074093672_value_8[1] = 9;
  state->vector_table_1074093672.write(8, vector_table_1074093672_value_8);
  // BDD node 41:vector_borrow(vector:(w64 1074076456), index:(w32 9), val_out:(w64 1074043656)[ -> (w64 1074090568)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074076456), index:(w32 9), value:(w64 1074090568)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_9(4);
  vector_table_1074076456_value_9[0] = 0;
  vector_table_1074076456_value_9[1] = 0;
  vector_table_1074076456_value_9[2] = 0;
  vector_table_1074076456_value_9[3] = 0;
  state->vector_table_1074076456.write(9, vector_table_1074076456_value_9);
  // BDD node 43:vector_borrow(vector:(w64 1074093672), index:(w32 9), val_out:(w64 1074043720)[ -> (w64 1074107784)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074093672), index:(w32 9), value:(w64 1074107784)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_9(2);
  vector_table_1074093672_value_9[0] = 0;
  vector_table_1074093672_value_9[1] = 8;
  state->vector_table_1074093672.write(9, vector_table_1074093672_value_9);
  // BDD node 45:vector_borrow(vector:(w64 1074076456), index:(w32 10), val_out:(w64 1074043656)[ -> (w64 1074090592)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074076456), index:(w32 10), value:(w64 1074090592)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_10(4);
  vector_table_1074076456_value_10[0] = 0;
  vector_table_1074076456_value_10[1] = 0;
  vector_table_1074076456_value_10[2] = 0;
  vector_table_1074076456_value_10[3] = 1;
  state->vector_table_1074076456.write(10, vector_table_1074076456_value_10);
  // BDD node 47:vector_borrow(vector:(w64 1074093672), index:(w32 10), val_out:(w64 1074043720)[ -> (w64 1074107808)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074093672), index:(w32 10), value:(w64 1074107808)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_10(2);
  vector_table_1074093672_value_10[0] = 0;
  vector_table_1074093672_value_10[1] = 11;
  state->vector_table_1074093672.write(10, vector_table_1074093672_value_10);
  // BDD node 49:vector_borrow(vector:(w64 1074076456), index:(w32 11), val_out:(w64 1074043656)[ -> (w64 1074090616)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074076456), index:(w32 11), value:(w64 1074090616)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_11(4);
  vector_table_1074076456_value_11[0] = 0;
  vector_table_1074076456_value_11[1] = 0;
  vector_table_1074076456_value_11[2] = 0;
  vector_table_1074076456_value_11[3] = 0;
  state->vector_table_1074076456.write(11, vector_table_1074076456_value_11);
  // BDD node 51:vector_borrow(vector:(w64 1074093672), index:(w32 11), val_out:(w64 1074043720)[ -> (w64 1074107832)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074093672), index:(w32 11), value:(w64 1074107832)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_11(2);
  vector_table_1074093672_value_11[0] = 0;
  vector_table_1074093672_value_11[1] = 10;
  state->vector_table_1074093672.write(11, vector_table_1074093672_value_11);
  // BDD node 53:vector_borrow(vector:(w64 1074076456), index:(w32 12), val_out:(w64 1074043656)[ -> (w64 1074090640)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074076456), index:(w32 12), value:(w64 1074090640)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_12(4);
  vector_table_1074076456_value_12[0] = 0;
  vector_table_1074076456_value_12[1] = 0;
  vector_table_1074076456_value_12[2] = 0;
  vector_table_1074076456_value_12[3] = 1;
  state->vector_table_1074076456.write(12, vector_table_1074076456_value_12);
  // BDD node 55:vector_borrow(vector:(w64 1074093672), index:(w32 12), val_out:(w64 1074043720)[ -> (w64 1074107856)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074093672), index:(w32 12), value:(w64 1074107856)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_12(2);
  vector_table_1074093672_value_12[0] = 0;
  vector_table_1074093672_value_12[1] = 13;
  state->vector_table_1074093672.write(12, vector_table_1074093672_value_12);
  // BDD node 57:vector_borrow(vector:(w64 1074076456), index:(w32 13), val_out:(w64 1074043656)[ -> (w64 1074090664)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074076456), index:(w32 13), value:(w64 1074090664)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_13(4);
  vector_table_1074076456_value_13[0] = 0;
  vector_table_1074076456_value_13[1] = 0;
  vector_table_1074076456_value_13[2] = 0;
  vector_table_1074076456_value_13[3] = 0;
  state->vector_table_1074076456.write(13, vector_table_1074076456_value_13);
  // BDD node 59:vector_borrow(vector:(w64 1074093672), index:(w32 13), val_out:(w64 1074043720)[ -> (w64 1074107880)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074093672), index:(w32 13), value:(w64 1074107880)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_13(2);
  vector_table_1074093672_value_13[0] = 0;
  vector_table_1074093672_value_13[1] = 12;
  state->vector_table_1074093672.write(13, vector_table_1074093672_value_13);
  // BDD node 61:vector_borrow(vector:(w64 1074076456), index:(w32 14), val_out:(w64 1074043656)[ -> (w64 1074090688)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074076456), index:(w32 14), value:(w64 1074090688)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_14(4);
  vector_table_1074076456_value_14[0] = 0;
  vector_table_1074076456_value_14[1] = 0;
  vector_table_1074076456_value_14[2] = 0;
  vector_table_1074076456_value_14[3] = 1;
  state->vector_table_1074076456.write(14, vector_table_1074076456_value_14);
  // BDD node 63:vector_borrow(vector:(w64 1074093672), index:(w32 14), val_out:(w64 1074043720)[ -> (w64 1074107904)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074093672), index:(w32 14), value:(w64 1074107904)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_14(2);
  vector_table_1074093672_value_14[0] = 0;
  vector_table_1074093672_value_14[1] = 15;
  state->vector_table_1074093672.write(14, vector_table_1074093672_value_14);
  // BDD node 65:vector_borrow(vector:(w64 1074076456), index:(w32 15), val_out:(w64 1074043656)[ -> (w64 1074090712)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074076456), index:(w32 15), value:(w64 1074090712)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_15(4);
  vector_table_1074076456_value_15[0] = 0;
  vector_table_1074076456_value_15[1] = 0;
  vector_table_1074076456_value_15[2] = 0;
  vector_table_1074076456_value_15[3] = 0;
  state->vector_table_1074076456.write(15, vector_table_1074076456_value_15);
  // BDD node 67:vector_borrow(vector:(w64 1074093672), index:(w32 15), val_out:(w64 1074043720)[ -> (w64 1074107928)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074093672), index:(w32 15), value:(w64 1074107928)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_15(2);
  vector_table_1074093672_value_15[0] = 0;
  vector_table_1074093672_value_15[1] = 14;
  state->vector_table_1074093672.write(15, vector_table_1074093672_value_15);
  // BDD node 69:vector_borrow(vector:(w64 1074076456), index:(w32 16), val_out:(w64 1074043656)[ -> (w64 1074090736)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074076456), index:(w32 16), value:(w64 1074090736)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_16(4);
  vector_table_1074076456_value_16[0] = 0;
  vector_table_1074076456_value_16[1] = 0;
  vector_table_1074076456_value_16[2] = 0;
  vector_table_1074076456_value_16[3] = 1;
  state->vector_table_1074076456.write(16, vector_table_1074076456_value_16);
  // BDD node 71:vector_borrow(vector:(w64 1074093672), index:(w32 16), val_out:(w64 1074043720)[ -> (w64 1074107952)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074093672), index:(w32 16), value:(w64 1074107952)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_16(2);
  vector_table_1074093672_value_16[0] = 0;
  vector_table_1074093672_value_16[1] = 17;
  state->vector_table_1074093672.write(16, vector_table_1074093672_value_16);
  // BDD node 73:vector_borrow(vector:(w64 1074076456), index:(w32 17), val_out:(w64 1074043656)[ -> (w64 1074090760)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074076456), index:(w32 17), value:(w64 1074090760)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_17(4);
  vector_table_1074076456_value_17[0] = 0;
  vector_table_1074076456_value_17[1] = 0;
  vector_table_1074076456_value_17[2] = 0;
  vector_table_1074076456_value_17[3] = 0;
  state->vector_table_1074076456.write(17, vector_table_1074076456_value_17);
  // BDD node 75:vector_borrow(vector:(w64 1074093672), index:(w32 17), val_out:(w64 1074043720)[ -> (w64 1074107976)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074093672), index:(w32 17), value:(w64 1074107976)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_17(2);
  vector_table_1074093672_value_17[0] = 0;
  vector_table_1074093672_value_17[1] = 16;
  state->vector_table_1074093672.write(17, vector_table_1074093672_value_17);
  // BDD node 77:vector_borrow(vector:(w64 1074076456), index:(w32 18), val_out:(w64 1074043656)[ -> (w64 1074090784)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074076456), index:(w32 18), value:(w64 1074090784)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_18(4);
  vector_table_1074076456_value_18[0] = 0;
  vector_table_1074076456_value_18[1] = 0;
  vector_table_1074076456_value_18[2] = 0;
  vector_table_1074076456_value_18[3] = 1;
  state->vector_table_1074076456.write(18, vector_table_1074076456_value_18);
  // BDD node 79:vector_borrow(vector:(w64 1074093672), index:(w32 18), val_out:(w64 1074043720)[ -> (w64 1074108000)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074093672), index:(w32 18), value:(w64 1074108000)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_18(2);
  vector_table_1074093672_value_18[0] = 0;
  vector_table_1074093672_value_18[1] = 19;
  state->vector_table_1074093672.write(18, vector_table_1074093672_value_18);
  // BDD node 81:vector_borrow(vector:(w64 1074076456), index:(w32 19), val_out:(w64 1074043656)[ -> (w64 1074090808)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074076456), index:(w32 19), value:(w64 1074090808)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_19(4);
  vector_table_1074076456_value_19[0] = 0;
  vector_table_1074076456_value_19[1] = 0;
  vector_table_1074076456_value_19[2] = 0;
  vector_table_1074076456_value_19[3] = 0;
  state->vector_table_1074076456.write(19, vector_table_1074076456_value_19);
  // BDD node 83:vector_borrow(vector:(w64 1074093672), index:(w32 19), val_out:(w64 1074043720)[ -> (w64 1074108024)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074093672), index:(w32 19), value:(w64 1074108024)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_19(2);
  vector_table_1074093672_value_19[0] = 0;
  vector_table_1074093672_value_19[1] = 18;
  state->vector_table_1074093672.write(19, vector_table_1074093672_value_19);
  // BDD node 85:vector_borrow(vector:(w64 1074076456), index:(w32 20), val_out:(w64 1074043656)[ -> (w64 1074090832)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074076456), index:(w32 20), value:(w64 1074090832)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_20(4);
  vector_table_1074076456_value_20[0] = 0;
  vector_table_1074076456_value_20[1] = 0;
  vector_table_1074076456_value_20[2] = 0;
  vector_table_1074076456_value_20[3] = 1;
  state->vector_table_1074076456.write(20, vector_table_1074076456_value_20);
  // BDD node 87:vector_borrow(vector:(w64 1074093672), index:(w32 20), val_out:(w64 1074043720)[ -> (w64 1074108048)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074093672), index:(w32 20), value:(w64 1074108048)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_20(2);
  vector_table_1074093672_value_20[0] = 0;
  vector_table_1074093672_value_20[1] = 21;
  state->vector_table_1074093672.write(20, vector_table_1074093672_value_20);
  // BDD node 89:vector_borrow(vector:(w64 1074076456), index:(w32 21), val_out:(w64 1074043656)[ -> (w64 1074090856)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074076456), index:(w32 21), value:(w64 1074090856)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_21(4);
  vector_table_1074076456_value_21[0] = 0;
  vector_table_1074076456_value_21[1] = 0;
  vector_table_1074076456_value_21[2] = 0;
  vector_table_1074076456_value_21[3] = 0;
  state->vector_table_1074076456.write(21, vector_table_1074076456_value_21);
  // BDD node 91:vector_borrow(vector:(w64 1074093672), index:(w32 21), val_out:(w64 1074043720)[ -> (w64 1074108072)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074093672), index:(w32 21), value:(w64 1074108072)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_21(2);
  vector_table_1074093672_value_21[0] = 0;
  vector_table_1074093672_value_21[1] = 20;
  state->vector_table_1074093672.write(21, vector_table_1074093672_value_21);
  // BDD node 93:vector_borrow(vector:(w64 1074076456), index:(w32 22), val_out:(w64 1074043656)[ -> (w64 1074090880)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074076456), index:(w32 22), value:(w64 1074090880)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_22(4);
  vector_table_1074076456_value_22[0] = 0;
  vector_table_1074076456_value_22[1] = 0;
  vector_table_1074076456_value_22[2] = 0;
  vector_table_1074076456_value_22[3] = 1;
  state->vector_table_1074076456.write(22, vector_table_1074076456_value_22);
  // BDD node 95:vector_borrow(vector:(w64 1074093672), index:(w32 22), val_out:(w64 1074043720)[ -> (w64 1074108096)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074093672), index:(w32 22), value:(w64 1074108096)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_22(2);
  vector_table_1074093672_value_22[0] = 0;
  vector_table_1074093672_value_22[1] = 23;
  state->vector_table_1074093672.write(22, vector_table_1074093672_value_22);
  // BDD node 97:vector_borrow(vector:(w64 1074076456), index:(w32 23), val_out:(w64 1074043656)[ -> (w64 1074090904)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074076456), index:(w32 23), value:(w64 1074090904)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_23(4);
  vector_table_1074076456_value_23[0] = 0;
  vector_table_1074076456_value_23[1] = 0;
  vector_table_1074076456_value_23[2] = 0;
  vector_table_1074076456_value_23[3] = 0;
  state->vector_table_1074076456.write(23, vector_table_1074076456_value_23);
  // BDD node 99:vector_borrow(vector:(w64 1074093672), index:(w32 23), val_out:(w64 1074043720)[ -> (w64 1074108120)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074093672), index:(w32 23), value:(w64 1074108120)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_23(2);
  vector_table_1074093672_value_23[0] = 0;
  vector_table_1074093672_value_23[1] = 22;
  state->vector_table_1074093672.write(23, vector_table_1074093672_value_23);
  // BDD node 101:vector_borrow(vector:(w64 1074076456), index:(w32 24), val_out:(w64 1074043656)[ -> (w64 1074090928)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074076456), index:(w32 24), value:(w64 1074090928)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_24(4);
  vector_table_1074076456_value_24[0] = 0;
  vector_table_1074076456_value_24[1] = 0;
  vector_table_1074076456_value_24[2] = 0;
  vector_table_1074076456_value_24[3] = 1;
  state->vector_table_1074076456.write(24, vector_table_1074076456_value_24);
  // BDD node 103:vector_borrow(vector:(w64 1074093672), index:(w32 24), val_out:(w64 1074043720)[ -> (w64 1074108144)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074093672), index:(w32 24), value:(w64 1074108144)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_24(2);
  vector_table_1074093672_value_24[0] = 0;
  vector_table_1074093672_value_24[1] = 25;
  state->vector_table_1074093672.write(24, vector_table_1074093672_value_24);
  // BDD node 105:vector_borrow(vector:(w64 1074076456), index:(w32 25), val_out:(w64 1074043656)[ -> (w64 1074090952)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074076456), index:(w32 25), value:(w64 1074090952)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_25(4);
  vector_table_1074076456_value_25[0] = 0;
  vector_table_1074076456_value_25[1] = 0;
  vector_table_1074076456_value_25[2] = 0;
  vector_table_1074076456_value_25[3] = 0;
  state->vector_table_1074076456.write(25, vector_table_1074076456_value_25);
  // BDD node 107:vector_borrow(vector:(w64 1074093672), index:(w32 25), val_out:(w64 1074043720)[ -> (w64 1074108168)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074093672), index:(w32 25), value:(w64 1074108168)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_25(2);
  vector_table_1074093672_value_25[0] = 0;
  vector_table_1074093672_value_25[1] = 24;
  state->vector_table_1074093672.write(25, vector_table_1074093672_value_25);
  // BDD node 109:vector_borrow(vector:(w64 1074076456), index:(w32 26), val_out:(w64 1074043656)[ -> (w64 1074090976)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074076456), index:(w32 26), value:(w64 1074090976)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_26(4);
  vector_table_1074076456_value_26[0] = 0;
  vector_table_1074076456_value_26[1] = 0;
  vector_table_1074076456_value_26[2] = 0;
  vector_table_1074076456_value_26[3] = 1;
  state->vector_table_1074076456.write(26, vector_table_1074076456_value_26);
  // BDD node 111:vector_borrow(vector:(w64 1074093672), index:(w32 26), val_out:(w64 1074043720)[ -> (w64 1074108192)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074093672), index:(w32 26), value:(w64 1074108192)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_26(2);
  vector_table_1074093672_value_26[0] = 0;
  vector_table_1074093672_value_26[1] = 27;
  state->vector_table_1074093672.write(26, vector_table_1074093672_value_26);
  // BDD node 113:vector_borrow(vector:(w64 1074076456), index:(w32 27), val_out:(w64 1074043656)[ -> (w64 1074091000)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074076456), index:(w32 27), value:(w64 1074091000)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_27(4);
  vector_table_1074076456_value_27[0] = 0;
  vector_table_1074076456_value_27[1] = 0;
  vector_table_1074076456_value_27[2] = 0;
  vector_table_1074076456_value_27[3] = 0;
  state->vector_table_1074076456.write(27, vector_table_1074076456_value_27);
  // BDD node 115:vector_borrow(vector:(w64 1074093672), index:(w32 27), val_out:(w64 1074043720)[ -> (w64 1074108216)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074093672), index:(w32 27), value:(w64 1074108216)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_27(2);
  vector_table_1074093672_value_27[0] = 0;
  vector_table_1074093672_value_27[1] = 26;
  state->vector_table_1074093672.write(27, vector_table_1074093672_value_27);
  // BDD node 117:vector_borrow(vector:(w64 1074076456), index:(w32 28), val_out:(w64 1074043656)[ -> (w64 1074091024)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074076456), index:(w32 28), value:(w64 1074091024)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_28(4);
  vector_table_1074076456_value_28[0] = 0;
  vector_table_1074076456_value_28[1] = 0;
  vector_table_1074076456_value_28[2] = 0;
  vector_table_1074076456_value_28[3] = 1;
  state->vector_table_1074076456.write(28, vector_table_1074076456_value_28);
  // BDD node 119:vector_borrow(vector:(w64 1074093672), index:(w32 28), val_out:(w64 1074043720)[ -> (w64 1074108240)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074093672), index:(w32 28), value:(w64 1074108240)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_28(2);
  vector_table_1074093672_value_28[0] = 0;
  vector_table_1074093672_value_28[1] = 29;
  state->vector_table_1074093672.write(28, vector_table_1074093672_value_28);
  // BDD node 121:vector_borrow(vector:(w64 1074076456), index:(w32 29), val_out:(w64 1074043656)[ -> (w64 1074091048)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074076456), index:(w32 29), value:(w64 1074091048)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_29(4);
  vector_table_1074076456_value_29[0] = 0;
  vector_table_1074076456_value_29[1] = 0;
  vector_table_1074076456_value_29[2] = 0;
  vector_table_1074076456_value_29[3] = 0;
  state->vector_table_1074076456.write(29, vector_table_1074076456_value_29);
  // BDD node 123:vector_borrow(vector:(w64 1074093672), index:(w32 29), val_out:(w64 1074043720)[ -> (w64 1074108264)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074093672), index:(w32 29), value:(w64 1074108264)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_29(2);
  vector_table_1074093672_value_29[0] = 0;
  vector_table_1074093672_value_29[1] = 28;
  state->vector_table_1074093672.write(29, vector_table_1074093672_value_29);
  // BDD node 125:vector_borrow(vector:(w64 1074076456), index:(w32 30), val_out:(w64 1074043656)[ -> (w64 1074091072)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074076456), index:(w32 30), value:(w64 1074091072)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_30(4);
  vector_table_1074076456_value_30[0] = 0;
  vector_table_1074076456_value_30[1] = 0;
  vector_table_1074076456_value_30[2] = 0;
  vector_table_1074076456_value_30[3] = 1;
  state->vector_table_1074076456.write(30, vector_table_1074076456_value_30);
  // BDD node 127:vector_borrow(vector:(w64 1074093672), index:(w32 30), val_out:(w64 1074043720)[ -> (w64 1074108288)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074093672), index:(w32 30), value:(w64 1074108288)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_30(2);
  vector_table_1074093672_value_30[0] = 0;
  vector_table_1074093672_value_30[1] = 31;
  state->vector_table_1074093672.write(30, vector_table_1074093672_value_30);
  // BDD node 129:vector_borrow(vector:(w64 1074076456), index:(w32 31), val_out:(w64 1074043656)[ -> (w64 1074091096)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074076456), index:(w32 31), value:(w64 1074091096)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076456_value_31(4);
  vector_table_1074076456_value_31[0] = 0;
  vector_table_1074076456_value_31[1] = 0;
  vector_table_1074076456_value_31[2] = 0;
  vector_table_1074076456_value_31[3] = 0;
  state->vector_table_1074076456.write(31, vector_table_1074076456_value_31);
  // BDD node 131:vector_borrow(vector:(w64 1074093672), index:(w32 31), val_out:(w64 1074043720)[ -> (w64 1074108312)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074093672), index:(w32 31), value:(w64 1074108312)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093672_value_31(2);
  vector_table_1074093672_value_31[0] = 0;
  vector_table_1074093672_value_31[1] = 30;
  state->vector_table_1074093672.write(31, vector_table_1074093672_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 vector_data__139;
  u32 map_has_this_key__159;
  u32 cached_insert_success;
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

  if (bswap16(cpu_hdr->code_path) == 10648) {
    // EP node  10631
    // BDD node 311:tofino_force_send_to_controller()
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  10632
    // BDD node 311:tofino_force_send_to_controller()
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  10633
    // BDD node 311:tofino_force_send_to_controller()
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  10634
    // BDD node 311:tofino_force_send_to_controller()
    buffer_t value_0;
    state->vector_table_1074076456.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  10635
    // BDD node 311:tofino_force_send_to_controller()
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  10636
      // BDD node 311:tofino_force_send_to_controller()
      // EP node  10638
      // BDD node 311:tofino_force_send_to_controller()
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    } else {
      // EP node  10637
      // BDD node 311:tofino_force_send_to_controller()
      // EP node  10639
      // BDD node 311:tofino_force_send_to_controller()
      buffer_t fcfs_cs_1074044048_key_0(13);
      fcfs_cs_1074044048_key_0[0] = *(u8*)(hdr_2 + 0);
      fcfs_cs_1074044048_key_0[1] = *(u8*)(hdr_2 + 1);
      fcfs_cs_1074044048_key_0[2] = *(u8*)(hdr_2 + 2);
      fcfs_cs_1074044048_key_0[3] = *(u8*)(hdr_2 + 3);
      fcfs_cs_1074044048_key_0[4] = *(u8*)(hdr_1 + 12);
      fcfs_cs_1074044048_key_0[5] = *(u8*)(hdr_1 + 13);
      fcfs_cs_1074044048_key_0[6] = *(u8*)(hdr_1 + 14);
      fcfs_cs_1074044048_key_0[7] = *(u8*)(hdr_1 + 15);
      fcfs_cs_1074044048_key_0[8] = *(u8*)(hdr_1 + 16);
      fcfs_cs_1074044048_key_0[9] = *(u8*)(hdr_1 + 17);
      fcfs_cs_1074044048_key_0[10] = *(u8*)(hdr_1 + 18);
      fcfs_cs_1074044048_key_0[11] = *(u8*)(hdr_1 + 19);
      fcfs_cs_1074044048_key_0[12] = *(u8*)(hdr_1 + 9);
      bool found_0 = state->fcfs_cs_1074044048.get(fcfs_cs_1074044048_key_0);
      // EP node  10640
      // BDD node 311:tofino_force_send_to_controller()
      if ((found_0) != (0)) {
        // EP node  10641
        // BDD node 311:tofino_force_send_to_controller()
        // EP node  10643
        // BDD node 311:tofino_force_send_to_controller()
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      } else {
        // EP node  10642
        // BDD node 311:tofino_force_send_to_controller()
        // EP node  10644
        // BDD node 311:tofino_force_send_to_controller()
        if ((bswap32(cpu_hdr_extra->cached_insert_success)) != (0)) {
          // EP node  10645
          // BDD node 311:tofino_force_send_to_controller()
          // EP node  10647
          // BDD node 311:tofino_force_send_to_controller()
          result.abort_transaction = true;
          cpu_hdr->trigger_dataplane_execution = 1;
          return result;
        } else {
          // EP node  10646
          // BDD node 311:tofino_force_send_to_controller()
          // EP node  15978
          // BDD node 288:dchain_allocate_new_index(chain:(w64 1074076032), index_out:(w64 1074217368)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index__288)], time:(ReadLSB w64 (w32 0) next_time))
          buffer_t fcfs_cs_1074044048_key_1(13);
          fcfs_cs_1074044048_key_1[0] = *(u8*)(hdr_2 + 0);
          fcfs_cs_1074044048_key_1[1] = *(u8*)(hdr_2 + 1);
          fcfs_cs_1074044048_key_1[2] = *(u8*)(hdr_2 + 2);
          fcfs_cs_1074044048_key_1[3] = *(u8*)(hdr_2 + 3);
          fcfs_cs_1074044048_key_1[4] = *(u8*)(hdr_1 + 12);
          fcfs_cs_1074044048_key_1[5] = *(u8*)(hdr_1 + 13);
          fcfs_cs_1074044048_key_1[6] = *(u8*)(hdr_1 + 14);
          fcfs_cs_1074044048_key_1[7] = *(u8*)(hdr_1 + 15);
          fcfs_cs_1074044048_key_1[8] = *(u8*)(hdr_1 + 16);
          fcfs_cs_1074044048_key_1[9] = *(u8*)(hdr_1 + 17);
          fcfs_cs_1074044048_key_1[10] = *(u8*)(hdr_1 + 18);
          fcfs_cs_1074044048_key_1[11] = *(u8*)(hdr_1 + 19);
          fcfs_cs_1074044048_key_1[12] = *(u8*)(hdr_1 + 9);
          bool success_0 = state->fcfs_cs_1074044048.put(fcfs_cs_1074044048_key_1);
          // EP node  16062
          // BDD node 289:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__288))
          if ((0) == (success_0)) {
            // EP node  16063
            // BDD node 289:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__288))
            // EP node  17167
            // BDD node 290:vector_borrow(vector:(w64 1074093672), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074217832)[ -> (w64 1074107568)])
            buffer_t value_1;
            state->vector_table_1074093672.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_1);
            // EP node  17558
            // BDD node 291:vector_return(vector:(w64 1074093672), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107568)[(ReadLSB w16 (w32 0) vector_data__163)])
            // EP node  18949
            // BDD node 295:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__163)))
            if ((bswap32(cpu_hdr_extra->DEVICE) & 65535) != ((u16)value_1.get(0, 2))) {
              // EP node  18950
              // BDD node 295:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__163)))
              // EP node  19053
              // BDD node 296:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__163)))
              if ((65535) != ((u16)value_1.get(0, 2))) {
                // EP node  19054
                // BDD node 296:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__163)))
                // EP node  19376
                // BDD node 297:FORWARD
                cpu_hdr->egress_dev = bswap16((u16)value_1.get(0, 2));
              } else {
                // EP node  19055
                // BDD node 296:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__163)))
                // EP node  19377
                // BDD node 298:DROP
                result.forward = false;
              }
            } else {
              // EP node  18951
              // BDD node 295:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__163)))
              // EP node  19160
              // BDD node 299:DROP
              result.forward = false;
            }
          } else {
            // EP node  16064
            // BDD node 289:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__288))
            // EP node  16149
            // BDD node 301:vector_borrow(vector:(w64 1074093672), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074220704)[ -> (w64 1074107568)])
            buffer_t value_2;
            state->vector_table_1074093672.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_2);
            // EP node  16325
            // BDD node 306:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__176)))
            if ((bswap32(cpu_hdr_extra->DEVICE) & 65535) != ((u16)value_2.get(0, 2))) {
              // EP node  16326
              // BDD node 306:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__176)))
              // EP node  16507
              // BDD node 302:vector_return(vector:(w64 1074093672), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107568)[(ReadLSB w16 (w32 0) vector_data__176)])
              // EP node  16691
              // BDD node 307:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__176)))
              if ((65535) != ((u16)value_2.get(0, 2))) {
                // EP node  16692
                // BDD node 307:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__176)))
                // EP node  17166
                // BDD node 308:FORWARD
                cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
              } else {
                // EP node  16693
                // BDD node 307:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__176)))
                // EP node  18548
                // BDD node 309:DROP
                result.forward = false;
              }
            } else {
              // EP node  16327
              // BDD node 306:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__176)))
              // EP node  17264
              // BDD node 325:vector_return(vector:(w64 1074093672), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107568)[(ReadLSB w16 (w32 0) vector_data__176)])
              // EP node  18848
              // BDD node 310:DROP
              result.forward = false;
            }
          }
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
