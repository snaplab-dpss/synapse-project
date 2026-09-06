#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  GuardedMapTable guarded_map_table_1074048392;
  VectorRegister vector_register_1074079432;
  DchainTable dchain_table_1074096568;
  BloomFilter bf_1074096984;
  VectorTable vector_table_1074109560;
  VectorTable vector_table_1074126776;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      guarded_map_table_1074048392("guarded_map_table_1074048392",{"Ingress.guarded_map_table_1074048392_145",},"Ingress.guarded_map_table_1074048392_guard", 1000LL),
      vector_register_1074079432("vector_register_1074079432",{"Ingress.vector_register_1074079432_0",}),
      dchain_table_1074096568("dchain_table_1074096568",{"Ingress.dchain_table_1074096568_167",}, 1000LL),
      bf_1074096984("bf_1074096984",{"Ingress.bf_1074096984_row_0", "Ingress.bf_1074096984_row_1", "Ingress.bf_1074096984_row_2", "Ingress.bf_1074096984_row_3", }, 10000LL),
      vector_table_1074109560("vector_table_1074109560",{"Ingress.vector_table_1074109560_142",}),
      vector_table_1074126776("vector_table_1074126776",{"Ingress.vector_table_1074126776_192","Ingress.vector_table_1074126776_270",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 4), map_out:(w64 1074048112)[(w64 0) -> (w64 1074048392)])
  // Module DataplaneGuardedMapTableAllocate
  // BDD node 2:vector_allocate(elem_size:(w32 4), capacity:(w32 65536), vector_out:(w64 1074048144)[(w64 0) -> (w64 1074079432)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 3:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074048128)[ -> (w64 1074096568)])
  // Module DataplaneDchainTableAllocate
  // BDD node 4:bf_allocate(height:(w32 4), width:(w32 1024), key_size:(w16 6), cleanup_interval:(w64 10000000000), bf_out:(w64 1074048136)[(w64 0) -> (w64 1074096984)])
  // Module DataplaneBloomFilterAllocate
  // BDD node 5:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074048152)[(w64 0) -> (w64 1074109560)])
  // Module DataplaneVectorTableAllocate
  // BDD node 6:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074048160)[(w64 0) -> (w64 1074126776)])
  // Module DataplaneVectorTableAllocate
  // BDD node 7:vector_borrow(vector:(w64 1074109560), index:(w32 0), val_out:(w64 1074047984)[ -> (w64 1074123456)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074109560), index:(w32 0), value:(w64 1074123456)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_0(4);
  vector_table_1074109560_value_0.set(0, 4, 1);
  state->vector_table_1074109560.write(0, vector_table_1074109560_value_0);
  // BDD node 9:vector_borrow(vector:(w64 1074126776), index:(w32 0), val_out:(w64 1074048048)[ -> (w64 1074140672)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074126776), index:(w32 0), value:(w64 1074140672)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_0(2);
  vector_table_1074126776_value_0.set(0, 2, 1);
  state->vector_table_1074126776.write(0, vector_table_1074126776_value_0);
  // BDD node 11:vector_borrow(vector:(w64 1074109560), index:(w32 1), val_out:(w64 1074047984)[ -> (w64 1074123480)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074109560), index:(w32 1), value:(w64 1074123480)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_1(4);
  vector_table_1074109560_value_1.set(0, 4, 0);
  state->vector_table_1074109560.write(1, vector_table_1074109560_value_1);
  // BDD node 13:vector_borrow(vector:(w64 1074126776), index:(w32 1), val_out:(w64 1074048048)[ -> (w64 1074140696)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074126776), index:(w32 1), value:(w64 1074140696)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_1(2);
  vector_table_1074126776_value_1.set(0, 2, 0);
  state->vector_table_1074126776.write(1, vector_table_1074126776_value_1);
  // BDD node 15:vector_borrow(vector:(w64 1074109560), index:(w32 2), val_out:(w64 1074047984)[ -> (w64 1074123504)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074109560), index:(w32 2), value:(w64 1074123504)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_2(4);
  vector_table_1074109560_value_2.set(0, 4, 1);
  state->vector_table_1074109560.write(2, vector_table_1074109560_value_2);
  // BDD node 17:vector_borrow(vector:(w64 1074126776), index:(w32 2), val_out:(w64 1074048048)[ -> (w64 1074140720)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074126776), index:(w32 2), value:(w64 1074140720)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_2(2);
  vector_table_1074126776_value_2.set(0, 2, 3);
  state->vector_table_1074126776.write(2, vector_table_1074126776_value_2);
  // BDD node 19:vector_borrow(vector:(w64 1074109560), index:(w32 3), val_out:(w64 1074047984)[ -> (w64 1074123528)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074109560), index:(w32 3), value:(w64 1074123528)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_3(4);
  vector_table_1074109560_value_3.set(0, 4, 0);
  state->vector_table_1074109560.write(3, vector_table_1074109560_value_3);
  // BDD node 21:vector_borrow(vector:(w64 1074126776), index:(w32 3), val_out:(w64 1074048048)[ -> (w64 1074140744)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074126776), index:(w32 3), value:(w64 1074140744)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_3(2);
  vector_table_1074126776_value_3.set(0, 2, 2);
  state->vector_table_1074126776.write(3, vector_table_1074126776_value_3);
  // BDD node 23:vector_borrow(vector:(w64 1074109560), index:(w32 4), val_out:(w64 1074047984)[ -> (w64 1074123552)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074109560), index:(w32 4), value:(w64 1074123552)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_4(4);
  vector_table_1074109560_value_4.set(0, 4, 1);
  state->vector_table_1074109560.write(4, vector_table_1074109560_value_4);
  // BDD node 25:vector_borrow(vector:(w64 1074126776), index:(w32 4), val_out:(w64 1074048048)[ -> (w64 1074140768)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074126776), index:(w32 4), value:(w64 1074140768)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_4(2);
  vector_table_1074126776_value_4.set(0, 2, 5);
  state->vector_table_1074126776.write(4, vector_table_1074126776_value_4);
  // BDD node 27:vector_borrow(vector:(w64 1074109560), index:(w32 5), val_out:(w64 1074047984)[ -> (w64 1074123576)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074109560), index:(w32 5), value:(w64 1074123576)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_5(4);
  vector_table_1074109560_value_5.set(0, 4, 0);
  state->vector_table_1074109560.write(5, vector_table_1074109560_value_5);
  // BDD node 29:vector_borrow(vector:(w64 1074126776), index:(w32 5), val_out:(w64 1074048048)[ -> (w64 1074140792)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074126776), index:(w32 5), value:(w64 1074140792)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_5(2);
  vector_table_1074126776_value_5.set(0, 2, 4);
  state->vector_table_1074126776.write(5, vector_table_1074126776_value_5);
  // BDD node 31:vector_borrow(vector:(w64 1074109560), index:(w32 6), val_out:(w64 1074047984)[ -> (w64 1074123600)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074109560), index:(w32 6), value:(w64 1074123600)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_6(4);
  vector_table_1074109560_value_6.set(0, 4, 1);
  state->vector_table_1074109560.write(6, vector_table_1074109560_value_6);
  // BDD node 33:vector_borrow(vector:(w64 1074126776), index:(w32 6), val_out:(w64 1074048048)[ -> (w64 1074140816)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074126776), index:(w32 6), value:(w64 1074140816)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_6(2);
  vector_table_1074126776_value_6.set(0, 2, 7);
  state->vector_table_1074126776.write(6, vector_table_1074126776_value_6);
  // BDD node 35:vector_borrow(vector:(w64 1074109560), index:(w32 7), val_out:(w64 1074047984)[ -> (w64 1074123624)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074109560), index:(w32 7), value:(w64 1074123624)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_7(4);
  vector_table_1074109560_value_7.set(0, 4, 0);
  state->vector_table_1074109560.write(7, vector_table_1074109560_value_7);
  // BDD node 37:vector_borrow(vector:(w64 1074126776), index:(w32 7), val_out:(w64 1074048048)[ -> (w64 1074140840)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074126776), index:(w32 7), value:(w64 1074140840)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_7(2);
  vector_table_1074126776_value_7.set(0, 2, 6);
  state->vector_table_1074126776.write(7, vector_table_1074126776_value_7);
  // BDD node 39:vector_borrow(vector:(w64 1074109560), index:(w32 8), val_out:(w64 1074047984)[ -> (w64 1074123648)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074109560), index:(w32 8), value:(w64 1074123648)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_8(4);
  vector_table_1074109560_value_8.set(0, 4, 1);
  state->vector_table_1074109560.write(8, vector_table_1074109560_value_8);
  // BDD node 41:vector_borrow(vector:(w64 1074126776), index:(w32 8), val_out:(w64 1074048048)[ -> (w64 1074140864)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074126776), index:(w32 8), value:(w64 1074140864)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_8(2);
  vector_table_1074126776_value_8.set(0, 2, 9);
  state->vector_table_1074126776.write(8, vector_table_1074126776_value_8);
  // BDD node 43:vector_borrow(vector:(w64 1074109560), index:(w32 9), val_out:(w64 1074047984)[ -> (w64 1074123672)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074109560), index:(w32 9), value:(w64 1074123672)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_9(4);
  vector_table_1074109560_value_9.set(0, 4, 0);
  state->vector_table_1074109560.write(9, vector_table_1074109560_value_9);
  // BDD node 45:vector_borrow(vector:(w64 1074126776), index:(w32 9), val_out:(w64 1074048048)[ -> (w64 1074140888)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074126776), index:(w32 9), value:(w64 1074140888)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_9(2);
  vector_table_1074126776_value_9.set(0, 2, 8);
  state->vector_table_1074126776.write(9, vector_table_1074126776_value_9);
  // BDD node 47:vector_borrow(vector:(w64 1074109560), index:(w32 10), val_out:(w64 1074047984)[ -> (w64 1074123696)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074109560), index:(w32 10), value:(w64 1074123696)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_10(4);
  vector_table_1074109560_value_10.set(0, 4, 1);
  state->vector_table_1074109560.write(10, vector_table_1074109560_value_10);
  // BDD node 49:vector_borrow(vector:(w64 1074126776), index:(w32 10), val_out:(w64 1074048048)[ -> (w64 1074140912)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074126776), index:(w32 10), value:(w64 1074140912)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_10(2);
  vector_table_1074126776_value_10.set(0, 2, 11);
  state->vector_table_1074126776.write(10, vector_table_1074126776_value_10);
  // BDD node 51:vector_borrow(vector:(w64 1074109560), index:(w32 11), val_out:(w64 1074047984)[ -> (w64 1074123720)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074109560), index:(w32 11), value:(w64 1074123720)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_11(4);
  vector_table_1074109560_value_11.set(0, 4, 0);
  state->vector_table_1074109560.write(11, vector_table_1074109560_value_11);
  // BDD node 53:vector_borrow(vector:(w64 1074126776), index:(w32 11), val_out:(w64 1074048048)[ -> (w64 1074140936)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074126776), index:(w32 11), value:(w64 1074140936)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_11(2);
  vector_table_1074126776_value_11.set(0, 2, 10);
  state->vector_table_1074126776.write(11, vector_table_1074126776_value_11);
  // BDD node 55:vector_borrow(vector:(w64 1074109560), index:(w32 12), val_out:(w64 1074047984)[ -> (w64 1074123744)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074109560), index:(w32 12), value:(w64 1074123744)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_12(4);
  vector_table_1074109560_value_12.set(0, 4, 1);
  state->vector_table_1074109560.write(12, vector_table_1074109560_value_12);
  // BDD node 57:vector_borrow(vector:(w64 1074126776), index:(w32 12), val_out:(w64 1074048048)[ -> (w64 1074140960)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074126776), index:(w32 12), value:(w64 1074140960)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_12(2);
  vector_table_1074126776_value_12.set(0, 2, 13);
  state->vector_table_1074126776.write(12, vector_table_1074126776_value_12);
  // BDD node 59:vector_borrow(vector:(w64 1074109560), index:(w32 13), val_out:(w64 1074047984)[ -> (w64 1074123768)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074109560), index:(w32 13), value:(w64 1074123768)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_13(4);
  vector_table_1074109560_value_13.set(0, 4, 0);
  state->vector_table_1074109560.write(13, vector_table_1074109560_value_13);
  // BDD node 61:vector_borrow(vector:(w64 1074126776), index:(w32 13), val_out:(w64 1074048048)[ -> (w64 1074140984)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074126776), index:(w32 13), value:(w64 1074140984)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_13(2);
  vector_table_1074126776_value_13.set(0, 2, 12);
  state->vector_table_1074126776.write(13, vector_table_1074126776_value_13);
  // BDD node 63:vector_borrow(vector:(w64 1074109560), index:(w32 14), val_out:(w64 1074047984)[ -> (w64 1074123792)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074109560), index:(w32 14), value:(w64 1074123792)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_14(4);
  vector_table_1074109560_value_14.set(0, 4, 1);
  state->vector_table_1074109560.write(14, vector_table_1074109560_value_14);
  // BDD node 65:vector_borrow(vector:(w64 1074126776), index:(w32 14), val_out:(w64 1074048048)[ -> (w64 1074141008)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074126776), index:(w32 14), value:(w64 1074141008)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_14(2);
  vector_table_1074126776_value_14.set(0, 2, 15);
  state->vector_table_1074126776.write(14, vector_table_1074126776_value_14);
  // BDD node 67:vector_borrow(vector:(w64 1074109560), index:(w32 15), val_out:(w64 1074047984)[ -> (w64 1074123816)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074109560), index:(w32 15), value:(w64 1074123816)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_15(4);
  vector_table_1074109560_value_15.set(0, 4, 0);
  state->vector_table_1074109560.write(15, vector_table_1074109560_value_15);
  // BDD node 69:vector_borrow(vector:(w64 1074126776), index:(w32 15), val_out:(w64 1074048048)[ -> (w64 1074141032)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074126776), index:(w32 15), value:(w64 1074141032)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_15(2);
  vector_table_1074126776_value_15.set(0, 2, 14);
  state->vector_table_1074126776.write(15, vector_table_1074126776_value_15);
  // BDD node 71:vector_borrow(vector:(w64 1074109560), index:(w32 16), val_out:(w64 1074047984)[ -> (w64 1074123840)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074109560), index:(w32 16), value:(w64 1074123840)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_16(4);
  vector_table_1074109560_value_16.set(0, 4, 1);
  state->vector_table_1074109560.write(16, vector_table_1074109560_value_16);
  // BDD node 73:vector_borrow(vector:(w64 1074126776), index:(w32 16), val_out:(w64 1074048048)[ -> (w64 1074141056)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074126776), index:(w32 16), value:(w64 1074141056)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_16(2);
  vector_table_1074126776_value_16.set(0, 2, 17);
  state->vector_table_1074126776.write(16, vector_table_1074126776_value_16);
  // BDD node 75:vector_borrow(vector:(w64 1074109560), index:(w32 17), val_out:(w64 1074047984)[ -> (w64 1074123864)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074109560), index:(w32 17), value:(w64 1074123864)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_17(4);
  vector_table_1074109560_value_17.set(0, 4, 0);
  state->vector_table_1074109560.write(17, vector_table_1074109560_value_17);
  // BDD node 77:vector_borrow(vector:(w64 1074126776), index:(w32 17), val_out:(w64 1074048048)[ -> (w64 1074141080)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074126776), index:(w32 17), value:(w64 1074141080)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_17(2);
  vector_table_1074126776_value_17.set(0, 2, 16);
  state->vector_table_1074126776.write(17, vector_table_1074126776_value_17);
  // BDD node 79:vector_borrow(vector:(w64 1074109560), index:(w32 18), val_out:(w64 1074047984)[ -> (w64 1074123888)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074109560), index:(w32 18), value:(w64 1074123888)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_18(4);
  vector_table_1074109560_value_18.set(0, 4, 1);
  state->vector_table_1074109560.write(18, vector_table_1074109560_value_18);
  // BDD node 81:vector_borrow(vector:(w64 1074126776), index:(w32 18), val_out:(w64 1074048048)[ -> (w64 1074141104)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074126776), index:(w32 18), value:(w64 1074141104)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_18(2);
  vector_table_1074126776_value_18.set(0, 2, 19);
  state->vector_table_1074126776.write(18, vector_table_1074126776_value_18);
  // BDD node 83:vector_borrow(vector:(w64 1074109560), index:(w32 19), val_out:(w64 1074047984)[ -> (w64 1074123912)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074109560), index:(w32 19), value:(w64 1074123912)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_19(4);
  vector_table_1074109560_value_19.set(0, 4, 0);
  state->vector_table_1074109560.write(19, vector_table_1074109560_value_19);
  // BDD node 85:vector_borrow(vector:(w64 1074126776), index:(w32 19), val_out:(w64 1074048048)[ -> (w64 1074141128)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074126776), index:(w32 19), value:(w64 1074141128)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_19(2);
  vector_table_1074126776_value_19.set(0, 2, 18);
  state->vector_table_1074126776.write(19, vector_table_1074126776_value_19);
  // BDD node 87:vector_borrow(vector:(w64 1074109560), index:(w32 20), val_out:(w64 1074047984)[ -> (w64 1074123936)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074109560), index:(w32 20), value:(w64 1074123936)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_20(4);
  vector_table_1074109560_value_20.set(0, 4, 1);
  state->vector_table_1074109560.write(20, vector_table_1074109560_value_20);
  // BDD node 89:vector_borrow(vector:(w64 1074126776), index:(w32 20), val_out:(w64 1074048048)[ -> (w64 1074141152)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074126776), index:(w32 20), value:(w64 1074141152)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_20(2);
  vector_table_1074126776_value_20.set(0, 2, 21);
  state->vector_table_1074126776.write(20, vector_table_1074126776_value_20);
  // BDD node 91:vector_borrow(vector:(w64 1074109560), index:(w32 21), val_out:(w64 1074047984)[ -> (w64 1074123960)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074109560), index:(w32 21), value:(w64 1074123960)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_21(4);
  vector_table_1074109560_value_21.set(0, 4, 0);
  state->vector_table_1074109560.write(21, vector_table_1074109560_value_21);
  // BDD node 93:vector_borrow(vector:(w64 1074126776), index:(w32 21), val_out:(w64 1074048048)[ -> (w64 1074141176)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074126776), index:(w32 21), value:(w64 1074141176)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_21(2);
  vector_table_1074126776_value_21.set(0, 2, 20);
  state->vector_table_1074126776.write(21, vector_table_1074126776_value_21);
  // BDD node 95:vector_borrow(vector:(w64 1074109560), index:(w32 22), val_out:(w64 1074047984)[ -> (w64 1074123984)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074109560), index:(w32 22), value:(w64 1074123984)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_22(4);
  vector_table_1074109560_value_22.set(0, 4, 1);
  state->vector_table_1074109560.write(22, vector_table_1074109560_value_22);
  // BDD node 97:vector_borrow(vector:(w64 1074126776), index:(w32 22), val_out:(w64 1074048048)[ -> (w64 1074141200)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074126776), index:(w32 22), value:(w64 1074141200)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_22(2);
  vector_table_1074126776_value_22.set(0, 2, 23);
  state->vector_table_1074126776.write(22, vector_table_1074126776_value_22);
  // BDD node 99:vector_borrow(vector:(w64 1074109560), index:(w32 23), val_out:(w64 1074047984)[ -> (w64 1074124008)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074109560), index:(w32 23), value:(w64 1074124008)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_23(4);
  vector_table_1074109560_value_23.set(0, 4, 0);
  state->vector_table_1074109560.write(23, vector_table_1074109560_value_23);
  // BDD node 101:vector_borrow(vector:(w64 1074126776), index:(w32 23), val_out:(w64 1074048048)[ -> (w64 1074141224)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074126776), index:(w32 23), value:(w64 1074141224)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_23(2);
  vector_table_1074126776_value_23.set(0, 2, 22);
  state->vector_table_1074126776.write(23, vector_table_1074126776_value_23);
  // BDD node 103:vector_borrow(vector:(w64 1074109560), index:(w32 24), val_out:(w64 1074047984)[ -> (w64 1074124032)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074109560), index:(w32 24), value:(w64 1074124032)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_24(4);
  vector_table_1074109560_value_24.set(0, 4, 1);
  state->vector_table_1074109560.write(24, vector_table_1074109560_value_24);
  // BDD node 105:vector_borrow(vector:(w64 1074126776), index:(w32 24), val_out:(w64 1074048048)[ -> (w64 1074141248)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074126776), index:(w32 24), value:(w64 1074141248)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_24(2);
  vector_table_1074126776_value_24.set(0, 2, 25);
  state->vector_table_1074126776.write(24, vector_table_1074126776_value_24);
  // BDD node 107:vector_borrow(vector:(w64 1074109560), index:(w32 25), val_out:(w64 1074047984)[ -> (w64 1074124056)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074109560), index:(w32 25), value:(w64 1074124056)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_25(4);
  vector_table_1074109560_value_25.set(0, 4, 0);
  state->vector_table_1074109560.write(25, vector_table_1074109560_value_25);
  // BDD node 109:vector_borrow(vector:(w64 1074126776), index:(w32 25), val_out:(w64 1074048048)[ -> (w64 1074141272)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074126776), index:(w32 25), value:(w64 1074141272)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_25(2);
  vector_table_1074126776_value_25.set(0, 2, 24);
  state->vector_table_1074126776.write(25, vector_table_1074126776_value_25);
  // BDD node 111:vector_borrow(vector:(w64 1074109560), index:(w32 26), val_out:(w64 1074047984)[ -> (w64 1074124080)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074109560), index:(w32 26), value:(w64 1074124080)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_26(4);
  vector_table_1074109560_value_26.set(0, 4, 1);
  state->vector_table_1074109560.write(26, vector_table_1074109560_value_26);
  // BDD node 113:vector_borrow(vector:(w64 1074126776), index:(w32 26), val_out:(w64 1074048048)[ -> (w64 1074141296)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074126776), index:(w32 26), value:(w64 1074141296)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_26(2);
  vector_table_1074126776_value_26.set(0, 2, 27);
  state->vector_table_1074126776.write(26, vector_table_1074126776_value_26);
  // BDD node 115:vector_borrow(vector:(w64 1074109560), index:(w32 27), val_out:(w64 1074047984)[ -> (w64 1074124104)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074109560), index:(w32 27), value:(w64 1074124104)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_27(4);
  vector_table_1074109560_value_27.set(0, 4, 0);
  state->vector_table_1074109560.write(27, vector_table_1074109560_value_27);
  // BDD node 117:vector_borrow(vector:(w64 1074126776), index:(w32 27), val_out:(w64 1074048048)[ -> (w64 1074141320)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074126776), index:(w32 27), value:(w64 1074141320)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_27(2);
  vector_table_1074126776_value_27.set(0, 2, 26);
  state->vector_table_1074126776.write(27, vector_table_1074126776_value_27);
  // BDD node 119:vector_borrow(vector:(w64 1074109560), index:(w32 28), val_out:(w64 1074047984)[ -> (w64 1074124128)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074109560), index:(w32 28), value:(w64 1074124128)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_28(4);
  vector_table_1074109560_value_28.set(0, 4, 1);
  state->vector_table_1074109560.write(28, vector_table_1074109560_value_28);
  // BDD node 121:vector_borrow(vector:(w64 1074126776), index:(w32 28), val_out:(w64 1074048048)[ -> (w64 1074141344)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074126776), index:(w32 28), value:(w64 1074141344)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_28(2);
  vector_table_1074126776_value_28.set(0, 2, 29);
  state->vector_table_1074126776.write(28, vector_table_1074126776_value_28);
  // BDD node 123:vector_borrow(vector:(w64 1074109560), index:(w32 29), val_out:(w64 1074047984)[ -> (w64 1074124152)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074109560), index:(w32 29), value:(w64 1074124152)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_29(4);
  vector_table_1074109560_value_29.set(0, 4, 0);
  state->vector_table_1074109560.write(29, vector_table_1074109560_value_29);
  // BDD node 125:vector_borrow(vector:(w64 1074126776), index:(w32 29), val_out:(w64 1074048048)[ -> (w64 1074141368)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074126776), index:(w32 29), value:(w64 1074141368)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_29(2);
  vector_table_1074126776_value_29.set(0, 2, 28);
  state->vector_table_1074126776.write(29, vector_table_1074126776_value_29);
  // BDD node 127:vector_borrow(vector:(w64 1074109560), index:(w32 30), val_out:(w64 1074047984)[ -> (w64 1074124176)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074109560), index:(w32 30), value:(w64 1074124176)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_30(4);
  vector_table_1074109560_value_30.set(0, 4, 1);
  state->vector_table_1074109560.write(30, vector_table_1074109560_value_30);
  // BDD node 129:vector_borrow(vector:(w64 1074126776), index:(w32 30), val_out:(w64 1074048048)[ -> (w64 1074141392)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074126776), index:(w32 30), value:(w64 1074141392)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_30(2);
  vector_table_1074126776_value_30.set(0, 2, 31);
  state->vector_table_1074126776.write(30, vector_table_1074126776_value_30);
  // BDD node 131:vector_borrow(vector:(w64 1074109560), index:(w32 31), val_out:(w64 1074047984)[ -> (w64 1074124200)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074109560), index:(w32 31), value:(w64 1074124200)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109560_value_31(4);
  vector_table_1074109560_value_31.set(0, 4, 0);
  state->vector_table_1074109560.write(31, vector_table_1074109560_value_31);
  // BDD node 133:vector_borrow(vector:(w64 1074126776), index:(w32 31), val_out:(w64 1074048048)[ -> (w64 1074141416)])
  // Module Ignore
  // BDD node 134:vector_return(vector:(w64 1074126776), index:(w32 31), value:(w64 1074141416)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126776_value_31(2);
  vector_table_1074126776_value_31.set(0, 2, 30);
  state->vector_table_1074126776.write(31, vector_table_1074126776_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 DEVICE;
  u32 bf_query_estimate__169;
  u32 vector_data__168;

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



  if (bswap16(cpu_hdr->code_path) == 2091) {
    // EP node  2078
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  2079
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  2080
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  2081
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t value_0;
    state->vector_table_1074109560.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  2082
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  2083
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  2086
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
      buffer_t guarded_map_table_1074048392_key_0(4);
      guarded_map_table_1074048392_key_0[0] = *(u8*)(hdr_1 + 12);
      guarded_map_table_1074048392_key_0[1] = *(u8*)(hdr_1 + 13);
      guarded_map_table_1074048392_key_0[2] = *(u8*)(hdr_1 + 14);
      guarded_map_table_1074048392_key_0[3] = *(u8*)(hdr_1 + 15);
      u32 value_1;
      bool found_0 = state->guarded_map_table_1074048392.get(guarded_map_table_1074048392_key_0, value_1);
      // EP node  2087
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_0)) {
        // EP node  2088
        // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  4676
        // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
        u32 allocated_index_0;
        bool success_0 = state->dchain_table_1074096568.allocate_new_index(allocated_index_0);
        // EP node  4748
        // BDD node 148:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__147))
        if ((0) == (success_0)) {
          // EP node  4749
          // BDD node 148:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__147))
          // EP node  6313
          // BDD node 149:vector_borrow(vector:(w64 1074126776), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074250024)[ -> (w64 1074140672)])
          buffer_t value_2;
          state->vector_table_1074126776.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_2);
          // EP node  6660
          // BDD node 150:vector_return(vector:(w64 1074126776), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074140672)[(ReadLSB w16 (w32 0) vector_data__149)])
          // EP node  7840
          // BDD node 154:FORWARD
          cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
        } else {
          // EP node  4750
          // BDD node 148:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__147))
          // EP node  5048
          // BDD node 156:map_put(map:(w64 1074048392), key:(w64 1074076112)[(ReadLSB w32 (w32 268) packet_chunks) -> (ReadLSB w32 (w32 268) packet_chunks)], value:(ReadLSB w32 (w32 0) new_index__147))
          buffer_t guarded_map_table_1074048392_key_1(4);
          guarded_map_table_1074048392_key_1[0] = *(u8*)(hdr_1 + 12);
          guarded_map_table_1074048392_key_1[1] = *(u8*)(hdr_1 + 13);
          guarded_map_table_1074048392_key_1[2] = *(u8*)(hdr_1 + 14);
          guarded_map_table_1074048392_key_1[3] = *(u8*)(hdr_1 + 15);
          state->guarded_map_table_1074048392.put(guarded_map_table_1074048392_key_1, allocated_index_0);
          // EP node  5276
          // BDD node 158:vector_borrow(vector:(w64 1074079432), index:(ReadLSB w32 (w32 0) new_index__147), val_out:(w64 1074249856)[ -> (w64 1074093328)])
          // EP node  5584
          // BDD node 159:vector_return(vector:(w64 1074079432), index:(ReadLSB w32 (w32 0) new_index__147), value:(w64 1074093328)[(w32 1)])
          buffer_t vector_register_1074079432_value_0(4);
          vector_register_1074079432_value_0.set(0, 4, 1);
          state->vector_register_1074079432.put(allocated_index_0, vector_register_1074079432_value_0);
          // EP node  5663
          // BDD node 160:bf_set(bf:(w64 1074096984), key:(w64 1074249874)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
          buffer_t bf_1074096984_key_0(6);
          bf_1074096984_key_0[0] = *(u8*)(hdr_1 + 12);
          bf_1074096984_key_0[1] = *(u8*)(hdr_1 + 13);
          bf_1074096984_key_0[2] = *(u8*)(hdr_1 + 14);
          bf_1074096984_key_0[3] = *(u8*)(hdr_1 + 15);
          bf_1074096984_key_0[4] = *(u8*)(hdr_2 + 2);
          bf_1074096984_key_0[5] = *(u8*)(hdr_2 + 3);
          state->bf_1074096984.set(bf_1074096984_key_0);
          // EP node  5742
          // BDD node 161:vector_borrow(vector:(w64 1074126776), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074252712)[ -> (w64 1074140672)])
          buffer_t value_3;
          state->vector_table_1074126776.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_3);
          // EP node  5822
          // BDD node 162:vector_return(vector:(w64 1074126776), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074140672)[(ReadLSB w16 (w32 0) vector_data__161)])
          // EP node  6227
          // BDD node 166:FORWARD
          cpu_hdr->egress_dev = bswap16((u16)value_3.get(0, 2));
        }
      } else {
        // EP node  2089
        // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  2090
        // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  2084
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  2085
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096568), index_out:(w64 1074249760)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    }
  }
  else if (bswap16(cpu_hdr->code_path) == 1849) {
    // EP node  1836
    // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
    u8* hdr_3 = packet_consume(pkt, 14);
    // EP node  1837
    // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
    u8* hdr_4 = packet_consume(pkt, 20);
    // EP node  1838
    // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
    u8* hdr_5 = packet_consume(pkt, 4);
    // EP node  1839
    // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
    buffer_t value_4;
    state->vector_table_1074109560.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_4);
    // EP node  1840
    // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
    if ((0) == ((u32)value_4.get(0, 4))) {
      // EP node  1841
      // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
      // EP node  1844
      // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
      buffer_t guarded_map_table_1074048392_key_2(4);
      guarded_map_table_1074048392_key_2[0] = *(u8*)(hdr_4 + 12);
      guarded_map_table_1074048392_key_2[1] = *(u8*)(hdr_4 + 13);
      guarded_map_table_1074048392_key_2[2] = *(u8*)(hdr_4 + 14);
      guarded_map_table_1074048392_key_2[3] = *(u8*)(hdr_4 + 15);
      u32 value_5;
      bool found_1 = state->guarded_map_table_1074048392.get(guarded_map_table_1074048392_key_2, value_5);
      // EP node  1845
      // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
      if ((0) == (found_1)) {
        // EP node  1846
        // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
        // EP node  1848
        // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      } else {
        // EP node  1847
        // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
        // EP node  3990
        // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
        buffer_t bf_1074096984_key_1(6);
        bf_1074096984_key_1[0] = *(u8*)(hdr_4 + 12);
        bf_1074096984_key_1[1] = *(u8*)(hdr_4 + 13);
        bf_1074096984_key_1[2] = *(u8*)(hdr_4 + 14);
        bf_1074096984_key_1[3] = *(u8*)(hdr_4 + 15);
        bf_1074096984_key_1[4] = *(u8*)(hdr_5 + 2);
        bf_1074096984_key_1[5] = *(u8*)(hdr_5 + 3);
        state->bf_1074096984.set(bf_1074096984_key_1);
        // EP node  4054
        // BDD node 171:if ((Eq (w32 0) (ReadLSB w32 (w32 0) bf_query_estimate__169))
        if ((0) == (bswap32(cpu_hdr_extra->bf_query_estimate__169))) {
          // EP node  4055
          // BDD node 171:if ((Eq (w32 0) (ReadLSB w32 (w32 0) bf_query_estimate__169))
          // EP node  6228
          // BDD node 172:if ((Ult (ReadLSB w32 (w32 0) vector_data__168) (w32 16))
          if ((bswap32(cpu_hdr_extra->vector_data__168)) < (16)) {
            // EP node  6229
            // BDD node 172:if ((Ult (ReadLSB w32 (w32 0) vector_data__168) (w32 16))
            // EP node  6571
            // BDD node 173:vector_return(vector:(w64 1074079432), index:(ReadLSB w32 (w32 0) allocated_index__145), value:(w64 1074093328)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__168))])
            buffer_t vector_register_1074079432_value_1(4);
            vector_register_1074079432_value_1.set(0, 4, (1) + (bswap32(cpu_hdr_extra->vector_data__168)));
            state->vector_register_1074079432.put(value_5, vector_register_1074079432_value_1);
            // EP node  6749
            // BDD node 174:vector_borrow(vector:(w64 1074126776), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074255080)[ -> (w64 1074140672)])
            buffer_t value_6;
            state->vector_table_1074126776.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_6);
            // EP node  7019
            // BDD node 175:vector_return(vector:(w64 1074126776), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074140672)[(ReadLSB w16 (w32 0) vector_data__174)])
            // EP node  8027
            // BDD node 179:FORWARD
            cpu_hdr->egress_dev = bswap16((u16)value_6.get(0, 2));
          } else {
            // EP node  6230
            // BDD node 172:if ((Ult (ReadLSB w32 (w32 0) vector_data__168) (w32 16))
            // EP node  6572
            // BDD node 180:vector_return(vector:(w64 1074079432), index:(ReadLSB w32 (w32 0) allocated_index__145), value:(w64 1074093328)[(ReadLSB w32 (w32 0) vector_data__168)])
            // EP node  7656
            // BDD node 184:DROP
            result.forward = false;
          }
        } else {
          // EP node  4056
          // BDD node 171:if ((Eq (w32 0) (ReadLSB w32 (w32 0) bf_query_estimate__169))
          // EP node  4188
          // BDD node 185:vector_return(vector:(w64 1074079432), index:(ReadLSB w32 (w32 0) allocated_index__145), value:(w64 1074093328)[(ReadLSB w32 (w32 0) vector_data__168)])
          // EP node  4256
          // BDD node 186:vector_borrow(vector:(w64 1074126776), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074257680)[ -> (w64 1074140672)])
          buffer_t value_7;
          state->vector_table_1074126776.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_7);
          // EP node  4325
          // BDD node 187:vector_return(vector:(w64 1074126776), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074140672)[(ReadLSB w16 (w32 0) vector_data__186)])
          // EP node  4675
          // BDD node 191:FORWARD
          cpu_hdr->egress_dev = bswap16((u16)value_7.get(0, 2));
        }
      }
    } else {
      // EP node  1842
      // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
      // EP node  1843
      // BDD node 170:bf_set(bf:(w64 1074096984), key:(w64 1074249362)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    }
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
