#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  MapTable map_table_1074048280;
  VectorRegister vector_register_1074079320;
  DchainTable dchain_table_1074096456;
  BloomFilter bf_1074096872;
  VectorTable vector_table_1074109448;
  VectorTable vector_table_1074126664;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      map_table_1074048280("map_table_1074048280",{"Ingress.map_table_1074048280_145",}, 1000LL),
      vector_register_1074079320("vector_register_1074079320",{"Ingress.vector_register_1074079320_0",}),
      dchain_table_1074096456("dchain_table_1074096456",{"Ingress.dchain_table_1074096456_171",}, 1000LL),
      bf_1074096872("bf_1074096872",{"Ingress.bf_1074096872_row_0", "Ingress.bf_1074096872_row_1", "Ingress.bf_1074096872_row_2", "Ingress.bf_1074096872_row_3", }, 10LL),
      vector_table_1074109448("vector_table_1074109448",{"Ingress.vector_table_1074109448_142",}),
      vector_table_1074126664("vector_table_1074126664",{"Ingress.vector_table_1074126664_200","Ingress.vector_table_1074126664_192","Ingress.vector_table_1074126664_178",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 4), map_out:(w64 1074048000)[(w64 0) -> (w64 1074048280)])
  // Module DataplaneMapTableAllocate
  // BDD node 2:vector_allocate(elem_size:(w32 4), capacity:(w32 65536), vector_out:(w64 1074048032)[(w64 0) -> (w64 1074079320)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 3:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074048016)[ -> (w64 1074096456)])
  // Module DataplaneDchainTableAllocate
  // BDD node 4:bf_allocate(height:(w32 4), width:(w32 1024), key_size:(w16 6), cleanup_interval:(w64 10000000), bf_out:(w64 1074048024)[(w64 0) -> (w64 1074096872)])
  // Module DataplaneBloomFilterAllocate
  // BDD node 5:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074048040)[(w64 0) -> (w64 1074109448)])
  // Module DataplaneVectorTableAllocate
  // BDD node 6:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074048048)[(w64 0) -> (w64 1074126664)])
  // Module DataplaneVectorTableAllocate
  // BDD node 7:vector_borrow(vector:(w64 1074109448), index:(w32 0), val_out:(w64 1074047872)[ -> (w64 1074123344)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074109448), index:(w32 0), value:(w64 1074123344)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_0(4);
  vector_table_1074109448_value_0[0] = 0;
  vector_table_1074109448_value_0[1] = 0;
  vector_table_1074109448_value_0[2] = 0;
  vector_table_1074109448_value_0[3] = 1;
  state->vector_table_1074109448.write(0, vector_table_1074109448_value_0);
  // BDD node 9:vector_borrow(vector:(w64 1074126664), index:(w32 0), val_out:(w64 1074047936)[ -> (w64 1074140560)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074126664), index:(w32 0), value:(w64 1074140560)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_0(2);
  vector_table_1074126664_value_0[0] = 0;
  vector_table_1074126664_value_0[1] = 1;
  state->vector_table_1074126664.write(0, vector_table_1074126664_value_0);
  // BDD node 11:vector_borrow(vector:(w64 1074109448), index:(w32 1), val_out:(w64 1074047872)[ -> (w64 1074123368)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074109448), index:(w32 1), value:(w64 1074123368)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_1(4);
  vector_table_1074109448_value_1[0] = 0;
  vector_table_1074109448_value_1[1] = 0;
  vector_table_1074109448_value_1[2] = 0;
  vector_table_1074109448_value_1[3] = 0;
  state->vector_table_1074109448.write(1, vector_table_1074109448_value_1);
  // BDD node 13:vector_borrow(vector:(w64 1074126664), index:(w32 1), val_out:(w64 1074047936)[ -> (w64 1074140584)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074126664), index:(w32 1), value:(w64 1074140584)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_1(2);
  vector_table_1074126664_value_1[0] = 0;
  vector_table_1074126664_value_1[1] = 0;
  state->vector_table_1074126664.write(1, vector_table_1074126664_value_1);
  // BDD node 15:vector_borrow(vector:(w64 1074109448), index:(w32 2), val_out:(w64 1074047872)[ -> (w64 1074123392)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074109448), index:(w32 2), value:(w64 1074123392)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_2(4);
  vector_table_1074109448_value_2[0] = 0;
  vector_table_1074109448_value_2[1] = 0;
  vector_table_1074109448_value_2[2] = 0;
  vector_table_1074109448_value_2[3] = 1;
  state->vector_table_1074109448.write(2, vector_table_1074109448_value_2);
  // BDD node 17:vector_borrow(vector:(w64 1074126664), index:(w32 2), val_out:(w64 1074047936)[ -> (w64 1074140608)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074126664), index:(w32 2), value:(w64 1074140608)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_2(2);
  vector_table_1074126664_value_2[0] = 0;
  vector_table_1074126664_value_2[1] = 3;
  state->vector_table_1074126664.write(2, vector_table_1074126664_value_2);
  // BDD node 19:vector_borrow(vector:(w64 1074109448), index:(w32 3), val_out:(w64 1074047872)[ -> (w64 1074123416)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074109448), index:(w32 3), value:(w64 1074123416)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_3(4);
  vector_table_1074109448_value_3[0] = 0;
  vector_table_1074109448_value_3[1] = 0;
  vector_table_1074109448_value_3[2] = 0;
  vector_table_1074109448_value_3[3] = 0;
  state->vector_table_1074109448.write(3, vector_table_1074109448_value_3);
  // BDD node 21:vector_borrow(vector:(w64 1074126664), index:(w32 3), val_out:(w64 1074047936)[ -> (w64 1074140632)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074126664), index:(w32 3), value:(w64 1074140632)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_3(2);
  vector_table_1074126664_value_3[0] = 0;
  vector_table_1074126664_value_3[1] = 2;
  state->vector_table_1074126664.write(3, vector_table_1074126664_value_3);
  // BDD node 23:vector_borrow(vector:(w64 1074109448), index:(w32 4), val_out:(w64 1074047872)[ -> (w64 1074123440)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074109448), index:(w32 4), value:(w64 1074123440)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_4(4);
  vector_table_1074109448_value_4[0] = 0;
  vector_table_1074109448_value_4[1] = 0;
  vector_table_1074109448_value_4[2] = 0;
  vector_table_1074109448_value_4[3] = 1;
  state->vector_table_1074109448.write(4, vector_table_1074109448_value_4);
  // BDD node 25:vector_borrow(vector:(w64 1074126664), index:(w32 4), val_out:(w64 1074047936)[ -> (w64 1074140656)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074126664), index:(w32 4), value:(w64 1074140656)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_4(2);
  vector_table_1074126664_value_4[0] = 0;
  vector_table_1074126664_value_4[1] = 5;
  state->vector_table_1074126664.write(4, vector_table_1074126664_value_4);
  // BDD node 27:vector_borrow(vector:(w64 1074109448), index:(w32 5), val_out:(w64 1074047872)[ -> (w64 1074123464)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074109448), index:(w32 5), value:(w64 1074123464)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_5(4);
  vector_table_1074109448_value_5[0] = 0;
  vector_table_1074109448_value_5[1] = 0;
  vector_table_1074109448_value_5[2] = 0;
  vector_table_1074109448_value_5[3] = 0;
  state->vector_table_1074109448.write(5, vector_table_1074109448_value_5);
  // BDD node 29:vector_borrow(vector:(w64 1074126664), index:(w32 5), val_out:(w64 1074047936)[ -> (w64 1074140680)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074126664), index:(w32 5), value:(w64 1074140680)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_5(2);
  vector_table_1074126664_value_5[0] = 0;
  vector_table_1074126664_value_5[1] = 4;
  state->vector_table_1074126664.write(5, vector_table_1074126664_value_5);
  // BDD node 31:vector_borrow(vector:(w64 1074109448), index:(w32 6), val_out:(w64 1074047872)[ -> (w64 1074123488)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074109448), index:(w32 6), value:(w64 1074123488)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_6(4);
  vector_table_1074109448_value_6[0] = 0;
  vector_table_1074109448_value_6[1] = 0;
  vector_table_1074109448_value_6[2] = 0;
  vector_table_1074109448_value_6[3] = 1;
  state->vector_table_1074109448.write(6, vector_table_1074109448_value_6);
  // BDD node 33:vector_borrow(vector:(w64 1074126664), index:(w32 6), val_out:(w64 1074047936)[ -> (w64 1074140704)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074126664), index:(w32 6), value:(w64 1074140704)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_6(2);
  vector_table_1074126664_value_6[0] = 0;
  vector_table_1074126664_value_6[1] = 7;
  state->vector_table_1074126664.write(6, vector_table_1074126664_value_6);
  // BDD node 35:vector_borrow(vector:(w64 1074109448), index:(w32 7), val_out:(w64 1074047872)[ -> (w64 1074123512)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074109448), index:(w32 7), value:(w64 1074123512)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_7(4);
  vector_table_1074109448_value_7[0] = 0;
  vector_table_1074109448_value_7[1] = 0;
  vector_table_1074109448_value_7[2] = 0;
  vector_table_1074109448_value_7[3] = 0;
  state->vector_table_1074109448.write(7, vector_table_1074109448_value_7);
  // BDD node 37:vector_borrow(vector:(w64 1074126664), index:(w32 7), val_out:(w64 1074047936)[ -> (w64 1074140728)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074126664), index:(w32 7), value:(w64 1074140728)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_7(2);
  vector_table_1074126664_value_7[0] = 0;
  vector_table_1074126664_value_7[1] = 6;
  state->vector_table_1074126664.write(7, vector_table_1074126664_value_7);
  // BDD node 39:vector_borrow(vector:(w64 1074109448), index:(w32 8), val_out:(w64 1074047872)[ -> (w64 1074123536)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074109448), index:(w32 8), value:(w64 1074123536)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_8(4);
  vector_table_1074109448_value_8[0] = 0;
  vector_table_1074109448_value_8[1] = 0;
  vector_table_1074109448_value_8[2] = 0;
  vector_table_1074109448_value_8[3] = 1;
  state->vector_table_1074109448.write(8, vector_table_1074109448_value_8);
  // BDD node 41:vector_borrow(vector:(w64 1074126664), index:(w32 8), val_out:(w64 1074047936)[ -> (w64 1074140752)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074126664), index:(w32 8), value:(w64 1074140752)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_8(2);
  vector_table_1074126664_value_8[0] = 0;
  vector_table_1074126664_value_8[1] = 9;
  state->vector_table_1074126664.write(8, vector_table_1074126664_value_8);
  // BDD node 43:vector_borrow(vector:(w64 1074109448), index:(w32 9), val_out:(w64 1074047872)[ -> (w64 1074123560)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074109448), index:(w32 9), value:(w64 1074123560)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_9(4);
  vector_table_1074109448_value_9[0] = 0;
  vector_table_1074109448_value_9[1] = 0;
  vector_table_1074109448_value_9[2] = 0;
  vector_table_1074109448_value_9[3] = 0;
  state->vector_table_1074109448.write(9, vector_table_1074109448_value_9);
  // BDD node 45:vector_borrow(vector:(w64 1074126664), index:(w32 9), val_out:(w64 1074047936)[ -> (w64 1074140776)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074126664), index:(w32 9), value:(w64 1074140776)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_9(2);
  vector_table_1074126664_value_9[0] = 0;
  vector_table_1074126664_value_9[1] = 8;
  state->vector_table_1074126664.write(9, vector_table_1074126664_value_9);
  // BDD node 47:vector_borrow(vector:(w64 1074109448), index:(w32 10), val_out:(w64 1074047872)[ -> (w64 1074123584)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074109448), index:(w32 10), value:(w64 1074123584)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_10(4);
  vector_table_1074109448_value_10[0] = 0;
  vector_table_1074109448_value_10[1] = 0;
  vector_table_1074109448_value_10[2] = 0;
  vector_table_1074109448_value_10[3] = 1;
  state->vector_table_1074109448.write(10, vector_table_1074109448_value_10);
  // BDD node 49:vector_borrow(vector:(w64 1074126664), index:(w32 10), val_out:(w64 1074047936)[ -> (w64 1074140800)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074126664), index:(w32 10), value:(w64 1074140800)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_10(2);
  vector_table_1074126664_value_10[0] = 0;
  vector_table_1074126664_value_10[1] = 11;
  state->vector_table_1074126664.write(10, vector_table_1074126664_value_10);
  // BDD node 51:vector_borrow(vector:(w64 1074109448), index:(w32 11), val_out:(w64 1074047872)[ -> (w64 1074123608)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074109448), index:(w32 11), value:(w64 1074123608)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_11(4);
  vector_table_1074109448_value_11[0] = 0;
  vector_table_1074109448_value_11[1] = 0;
  vector_table_1074109448_value_11[2] = 0;
  vector_table_1074109448_value_11[3] = 0;
  state->vector_table_1074109448.write(11, vector_table_1074109448_value_11);
  // BDD node 53:vector_borrow(vector:(w64 1074126664), index:(w32 11), val_out:(w64 1074047936)[ -> (w64 1074140824)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074126664), index:(w32 11), value:(w64 1074140824)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_11(2);
  vector_table_1074126664_value_11[0] = 0;
  vector_table_1074126664_value_11[1] = 10;
  state->vector_table_1074126664.write(11, vector_table_1074126664_value_11);
  // BDD node 55:vector_borrow(vector:(w64 1074109448), index:(w32 12), val_out:(w64 1074047872)[ -> (w64 1074123632)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074109448), index:(w32 12), value:(w64 1074123632)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_12(4);
  vector_table_1074109448_value_12[0] = 0;
  vector_table_1074109448_value_12[1] = 0;
  vector_table_1074109448_value_12[2] = 0;
  vector_table_1074109448_value_12[3] = 1;
  state->vector_table_1074109448.write(12, vector_table_1074109448_value_12);
  // BDD node 57:vector_borrow(vector:(w64 1074126664), index:(w32 12), val_out:(w64 1074047936)[ -> (w64 1074140848)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074126664), index:(w32 12), value:(w64 1074140848)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_12(2);
  vector_table_1074126664_value_12[0] = 0;
  vector_table_1074126664_value_12[1] = 13;
  state->vector_table_1074126664.write(12, vector_table_1074126664_value_12);
  // BDD node 59:vector_borrow(vector:(w64 1074109448), index:(w32 13), val_out:(w64 1074047872)[ -> (w64 1074123656)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074109448), index:(w32 13), value:(w64 1074123656)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_13(4);
  vector_table_1074109448_value_13[0] = 0;
  vector_table_1074109448_value_13[1] = 0;
  vector_table_1074109448_value_13[2] = 0;
  vector_table_1074109448_value_13[3] = 0;
  state->vector_table_1074109448.write(13, vector_table_1074109448_value_13);
  // BDD node 61:vector_borrow(vector:(w64 1074126664), index:(w32 13), val_out:(w64 1074047936)[ -> (w64 1074140872)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074126664), index:(w32 13), value:(w64 1074140872)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_13(2);
  vector_table_1074126664_value_13[0] = 0;
  vector_table_1074126664_value_13[1] = 12;
  state->vector_table_1074126664.write(13, vector_table_1074126664_value_13);
  // BDD node 63:vector_borrow(vector:(w64 1074109448), index:(w32 14), val_out:(w64 1074047872)[ -> (w64 1074123680)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074109448), index:(w32 14), value:(w64 1074123680)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_14(4);
  vector_table_1074109448_value_14[0] = 0;
  vector_table_1074109448_value_14[1] = 0;
  vector_table_1074109448_value_14[2] = 0;
  vector_table_1074109448_value_14[3] = 1;
  state->vector_table_1074109448.write(14, vector_table_1074109448_value_14);
  // BDD node 65:vector_borrow(vector:(w64 1074126664), index:(w32 14), val_out:(w64 1074047936)[ -> (w64 1074140896)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074126664), index:(w32 14), value:(w64 1074140896)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_14(2);
  vector_table_1074126664_value_14[0] = 0;
  vector_table_1074126664_value_14[1] = 15;
  state->vector_table_1074126664.write(14, vector_table_1074126664_value_14);
  // BDD node 67:vector_borrow(vector:(w64 1074109448), index:(w32 15), val_out:(w64 1074047872)[ -> (w64 1074123704)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074109448), index:(w32 15), value:(w64 1074123704)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_15(4);
  vector_table_1074109448_value_15[0] = 0;
  vector_table_1074109448_value_15[1] = 0;
  vector_table_1074109448_value_15[2] = 0;
  vector_table_1074109448_value_15[3] = 0;
  state->vector_table_1074109448.write(15, vector_table_1074109448_value_15);
  // BDD node 69:vector_borrow(vector:(w64 1074126664), index:(w32 15), val_out:(w64 1074047936)[ -> (w64 1074140920)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074126664), index:(w32 15), value:(w64 1074140920)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_15(2);
  vector_table_1074126664_value_15[0] = 0;
  vector_table_1074126664_value_15[1] = 14;
  state->vector_table_1074126664.write(15, vector_table_1074126664_value_15);
  // BDD node 71:vector_borrow(vector:(w64 1074109448), index:(w32 16), val_out:(w64 1074047872)[ -> (w64 1074123728)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074109448), index:(w32 16), value:(w64 1074123728)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_16(4);
  vector_table_1074109448_value_16[0] = 0;
  vector_table_1074109448_value_16[1] = 0;
  vector_table_1074109448_value_16[2] = 0;
  vector_table_1074109448_value_16[3] = 1;
  state->vector_table_1074109448.write(16, vector_table_1074109448_value_16);
  // BDD node 73:vector_borrow(vector:(w64 1074126664), index:(w32 16), val_out:(w64 1074047936)[ -> (w64 1074140944)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074126664), index:(w32 16), value:(w64 1074140944)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_16(2);
  vector_table_1074126664_value_16[0] = 0;
  vector_table_1074126664_value_16[1] = 17;
  state->vector_table_1074126664.write(16, vector_table_1074126664_value_16);
  // BDD node 75:vector_borrow(vector:(w64 1074109448), index:(w32 17), val_out:(w64 1074047872)[ -> (w64 1074123752)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074109448), index:(w32 17), value:(w64 1074123752)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_17(4);
  vector_table_1074109448_value_17[0] = 0;
  vector_table_1074109448_value_17[1] = 0;
  vector_table_1074109448_value_17[2] = 0;
  vector_table_1074109448_value_17[3] = 0;
  state->vector_table_1074109448.write(17, vector_table_1074109448_value_17);
  // BDD node 77:vector_borrow(vector:(w64 1074126664), index:(w32 17), val_out:(w64 1074047936)[ -> (w64 1074140968)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074126664), index:(w32 17), value:(w64 1074140968)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_17(2);
  vector_table_1074126664_value_17[0] = 0;
  vector_table_1074126664_value_17[1] = 16;
  state->vector_table_1074126664.write(17, vector_table_1074126664_value_17);
  // BDD node 79:vector_borrow(vector:(w64 1074109448), index:(w32 18), val_out:(w64 1074047872)[ -> (w64 1074123776)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074109448), index:(w32 18), value:(w64 1074123776)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_18(4);
  vector_table_1074109448_value_18[0] = 0;
  vector_table_1074109448_value_18[1] = 0;
  vector_table_1074109448_value_18[2] = 0;
  vector_table_1074109448_value_18[3] = 1;
  state->vector_table_1074109448.write(18, vector_table_1074109448_value_18);
  // BDD node 81:vector_borrow(vector:(w64 1074126664), index:(w32 18), val_out:(w64 1074047936)[ -> (w64 1074140992)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074126664), index:(w32 18), value:(w64 1074140992)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_18(2);
  vector_table_1074126664_value_18[0] = 0;
  vector_table_1074126664_value_18[1] = 19;
  state->vector_table_1074126664.write(18, vector_table_1074126664_value_18);
  // BDD node 83:vector_borrow(vector:(w64 1074109448), index:(w32 19), val_out:(w64 1074047872)[ -> (w64 1074123800)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074109448), index:(w32 19), value:(w64 1074123800)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_19(4);
  vector_table_1074109448_value_19[0] = 0;
  vector_table_1074109448_value_19[1] = 0;
  vector_table_1074109448_value_19[2] = 0;
  vector_table_1074109448_value_19[3] = 0;
  state->vector_table_1074109448.write(19, vector_table_1074109448_value_19);
  // BDD node 85:vector_borrow(vector:(w64 1074126664), index:(w32 19), val_out:(w64 1074047936)[ -> (w64 1074141016)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074126664), index:(w32 19), value:(w64 1074141016)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_19(2);
  vector_table_1074126664_value_19[0] = 0;
  vector_table_1074126664_value_19[1] = 18;
  state->vector_table_1074126664.write(19, vector_table_1074126664_value_19);
  // BDD node 87:vector_borrow(vector:(w64 1074109448), index:(w32 20), val_out:(w64 1074047872)[ -> (w64 1074123824)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074109448), index:(w32 20), value:(w64 1074123824)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_20(4);
  vector_table_1074109448_value_20[0] = 0;
  vector_table_1074109448_value_20[1] = 0;
  vector_table_1074109448_value_20[2] = 0;
  vector_table_1074109448_value_20[3] = 1;
  state->vector_table_1074109448.write(20, vector_table_1074109448_value_20);
  // BDD node 89:vector_borrow(vector:(w64 1074126664), index:(w32 20), val_out:(w64 1074047936)[ -> (w64 1074141040)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074126664), index:(w32 20), value:(w64 1074141040)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_20(2);
  vector_table_1074126664_value_20[0] = 0;
  vector_table_1074126664_value_20[1] = 21;
  state->vector_table_1074126664.write(20, vector_table_1074126664_value_20);
  // BDD node 91:vector_borrow(vector:(w64 1074109448), index:(w32 21), val_out:(w64 1074047872)[ -> (w64 1074123848)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074109448), index:(w32 21), value:(w64 1074123848)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_21(4);
  vector_table_1074109448_value_21[0] = 0;
  vector_table_1074109448_value_21[1] = 0;
  vector_table_1074109448_value_21[2] = 0;
  vector_table_1074109448_value_21[3] = 0;
  state->vector_table_1074109448.write(21, vector_table_1074109448_value_21);
  // BDD node 93:vector_borrow(vector:(w64 1074126664), index:(w32 21), val_out:(w64 1074047936)[ -> (w64 1074141064)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074126664), index:(w32 21), value:(w64 1074141064)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_21(2);
  vector_table_1074126664_value_21[0] = 0;
  vector_table_1074126664_value_21[1] = 20;
  state->vector_table_1074126664.write(21, vector_table_1074126664_value_21);
  // BDD node 95:vector_borrow(vector:(w64 1074109448), index:(w32 22), val_out:(w64 1074047872)[ -> (w64 1074123872)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074109448), index:(w32 22), value:(w64 1074123872)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_22(4);
  vector_table_1074109448_value_22[0] = 0;
  vector_table_1074109448_value_22[1] = 0;
  vector_table_1074109448_value_22[2] = 0;
  vector_table_1074109448_value_22[3] = 1;
  state->vector_table_1074109448.write(22, vector_table_1074109448_value_22);
  // BDD node 97:vector_borrow(vector:(w64 1074126664), index:(w32 22), val_out:(w64 1074047936)[ -> (w64 1074141088)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074126664), index:(w32 22), value:(w64 1074141088)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_22(2);
  vector_table_1074126664_value_22[0] = 0;
  vector_table_1074126664_value_22[1] = 23;
  state->vector_table_1074126664.write(22, vector_table_1074126664_value_22);
  // BDD node 99:vector_borrow(vector:(w64 1074109448), index:(w32 23), val_out:(w64 1074047872)[ -> (w64 1074123896)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074109448), index:(w32 23), value:(w64 1074123896)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_23(4);
  vector_table_1074109448_value_23[0] = 0;
  vector_table_1074109448_value_23[1] = 0;
  vector_table_1074109448_value_23[2] = 0;
  vector_table_1074109448_value_23[3] = 0;
  state->vector_table_1074109448.write(23, vector_table_1074109448_value_23);
  // BDD node 101:vector_borrow(vector:(w64 1074126664), index:(w32 23), val_out:(w64 1074047936)[ -> (w64 1074141112)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074126664), index:(w32 23), value:(w64 1074141112)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_23(2);
  vector_table_1074126664_value_23[0] = 0;
  vector_table_1074126664_value_23[1] = 22;
  state->vector_table_1074126664.write(23, vector_table_1074126664_value_23);
  // BDD node 103:vector_borrow(vector:(w64 1074109448), index:(w32 24), val_out:(w64 1074047872)[ -> (w64 1074123920)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074109448), index:(w32 24), value:(w64 1074123920)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_24(4);
  vector_table_1074109448_value_24[0] = 0;
  vector_table_1074109448_value_24[1] = 0;
  vector_table_1074109448_value_24[2] = 0;
  vector_table_1074109448_value_24[3] = 1;
  state->vector_table_1074109448.write(24, vector_table_1074109448_value_24);
  // BDD node 105:vector_borrow(vector:(w64 1074126664), index:(w32 24), val_out:(w64 1074047936)[ -> (w64 1074141136)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074126664), index:(w32 24), value:(w64 1074141136)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_24(2);
  vector_table_1074126664_value_24[0] = 0;
  vector_table_1074126664_value_24[1] = 25;
  state->vector_table_1074126664.write(24, vector_table_1074126664_value_24);
  // BDD node 107:vector_borrow(vector:(w64 1074109448), index:(w32 25), val_out:(w64 1074047872)[ -> (w64 1074123944)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074109448), index:(w32 25), value:(w64 1074123944)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_25(4);
  vector_table_1074109448_value_25[0] = 0;
  vector_table_1074109448_value_25[1] = 0;
  vector_table_1074109448_value_25[2] = 0;
  vector_table_1074109448_value_25[3] = 0;
  state->vector_table_1074109448.write(25, vector_table_1074109448_value_25);
  // BDD node 109:vector_borrow(vector:(w64 1074126664), index:(w32 25), val_out:(w64 1074047936)[ -> (w64 1074141160)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074126664), index:(w32 25), value:(w64 1074141160)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_25(2);
  vector_table_1074126664_value_25[0] = 0;
  vector_table_1074126664_value_25[1] = 24;
  state->vector_table_1074126664.write(25, vector_table_1074126664_value_25);
  // BDD node 111:vector_borrow(vector:(w64 1074109448), index:(w32 26), val_out:(w64 1074047872)[ -> (w64 1074123968)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074109448), index:(w32 26), value:(w64 1074123968)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_26(4);
  vector_table_1074109448_value_26[0] = 0;
  vector_table_1074109448_value_26[1] = 0;
  vector_table_1074109448_value_26[2] = 0;
  vector_table_1074109448_value_26[3] = 1;
  state->vector_table_1074109448.write(26, vector_table_1074109448_value_26);
  // BDD node 113:vector_borrow(vector:(w64 1074126664), index:(w32 26), val_out:(w64 1074047936)[ -> (w64 1074141184)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074126664), index:(w32 26), value:(w64 1074141184)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_26(2);
  vector_table_1074126664_value_26[0] = 0;
  vector_table_1074126664_value_26[1] = 27;
  state->vector_table_1074126664.write(26, vector_table_1074126664_value_26);
  // BDD node 115:vector_borrow(vector:(w64 1074109448), index:(w32 27), val_out:(w64 1074047872)[ -> (w64 1074123992)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074109448), index:(w32 27), value:(w64 1074123992)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_27(4);
  vector_table_1074109448_value_27[0] = 0;
  vector_table_1074109448_value_27[1] = 0;
  vector_table_1074109448_value_27[2] = 0;
  vector_table_1074109448_value_27[3] = 0;
  state->vector_table_1074109448.write(27, vector_table_1074109448_value_27);
  // BDD node 117:vector_borrow(vector:(w64 1074126664), index:(w32 27), val_out:(w64 1074047936)[ -> (w64 1074141208)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074126664), index:(w32 27), value:(w64 1074141208)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_27(2);
  vector_table_1074126664_value_27[0] = 0;
  vector_table_1074126664_value_27[1] = 26;
  state->vector_table_1074126664.write(27, vector_table_1074126664_value_27);
  // BDD node 119:vector_borrow(vector:(w64 1074109448), index:(w32 28), val_out:(w64 1074047872)[ -> (w64 1074124016)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074109448), index:(w32 28), value:(w64 1074124016)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_28(4);
  vector_table_1074109448_value_28[0] = 0;
  vector_table_1074109448_value_28[1] = 0;
  vector_table_1074109448_value_28[2] = 0;
  vector_table_1074109448_value_28[3] = 1;
  state->vector_table_1074109448.write(28, vector_table_1074109448_value_28);
  // BDD node 121:vector_borrow(vector:(w64 1074126664), index:(w32 28), val_out:(w64 1074047936)[ -> (w64 1074141232)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074126664), index:(w32 28), value:(w64 1074141232)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_28(2);
  vector_table_1074126664_value_28[0] = 0;
  vector_table_1074126664_value_28[1] = 29;
  state->vector_table_1074126664.write(28, vector_table_1074126664_value_28);
  // BDD node 123:vector_borrow(vector:(w64 1074109448), index:(w32 29), val_out:(w64 1074047872)[ -> (w64 1074124040)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074109448), index:(w32 29), value:(w64 1074124040)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_29(4);
  vector_table_1074109448_value_29[0] = 0;
  vector_table_1074109448_value_29[1] = 0;
  vector_table_1074109448_value_29[2] = 0;
  vector_table_1074109448_value_29[3] = 0;
  state->vector_table_1074109448.write(29, vector_table_1074109448_value_29);
  // BDD node 125:vector_borrow(vector:(w64 1074126664), index:(w32 29), val_out:(w64 1074047936)[ -> (w64 1074141256)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074126664), index:(w32 29), value:(w64 1074141256)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_29(2);
  vector_table_1074126664_value_29[0] = 0;
  vector_table_1074126664_value_29[1] = 28;
  state->vector_table_1074126664.write(29, vector_table_1074126664_value_29);
  // BDD node 127:vector_borrow(vector:(w64 1074109448), index:(w32 30), val_out:(w64 1074047872)[ -> (w64 1074124064)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074109448), index:(w32 30), value:(w64 1074124064)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_30(4);
  vector_table_1074109448_value_30[0] = 0;
  vector_table_1074109448_value_30[1] = 0;
  vector_table_1074109448_value_30[2] = 0;
  vector_table_1074109448_value_30[3] = 1;
  state->vector_table_1074109448.write(30, vector_table_1074109448_value_30);
  // BDD node 129:vector_borrow(vector:(w64 1074126664), index:(w32 30), val_out:(w64 1074047936)[ -> (w64 1074141280)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074126664), index:(w32 30), value:(w64 1074141280)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_30(2);
  vector_table_1074126664_value_30[0] = 0;
  vector_table_1074126664_value_30[1] = 31;
  state->vector_table_1074126664.write(30, vector_table_1074126664_value_30);
  // BDD node 131:vector_borrow(vector:(w64 1074109448), index:(w32 31), val_out:(w64 1074047872)[ -> (w64 1074124088)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074109448), index:(w32 31), value:(w64 1074124088)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074109448_value_31(4);
  vector_table_1074109448_value_31[0] = 0;
  vector_table_1074109448_value_31[1] = 0;
  vector_table_1074109448_value_31[2] = 0;
  vector_table_1074109448_value_31[3] = 0;
  state->vector_table_1074109448.write(31, vector_table_1074109448_value_31);
  // BDD node 133:vector_borrow(vector:(w64 1074126664), index:(w32 31), val_out:(w64 1074047936)[ -> (w64 1074141304)])
  // Module Ignore
  // BDD node 134:vector_return(vector:(w64 1074126664), index:(w32 31), value:(w64 1074141304)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074126664_value_31(2);
  vector_table_1074126664_value_31[0] = 0;
  vector_table_1074126664_value_31[1] = 30;
  state->vector_table_1074126664.write(31, vector_table_1074126664_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 map_has_this_key__145;
  u32 vector_data__142;
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

  if (bswap16(cpu_hdr->code_path) == 5675) {
    // EP node  5662
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  5663
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  5664
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  5665
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t value_0;
    state->vector_table_1074109448.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  5666
    // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  5667
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  5670
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
      buffer_t map_table_1074048280_key_0(4);
      map_table_1074048280_key_0[0] = *(u8*)(hdr_1 + 12);
      map_table_1074048280_key_0[1] = *(u8*)(hdr_1 + 13);
      map_table_1074048280_key_0[2] = *(u8*)(hdr_1 + 14);
      map_table_1074048280_key_0[3] = *(u8*)(hdr_1 + 15);
      u32 value_1;
      bool found_0 = state->map_table_1074048280.get(map_table_1074048280_key_0, value_1);
      // EP node  5671
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_0)) {
        // EP node  5672
        // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  7931
        // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
        u32 allocated_index_0;
        bool success_0 = state->dchain_table_1074096456.allocate_new_index(allocated_index_0);
        // EP node  8001
        // BDD node 148:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__147))
        if ((0) == (success_0)) {
          // EP node  8002
          // BDD node 148:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__147))
          // EP node  10376
          // BDD node 149:vector_borrow(vector:(w64 1074126664), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074250632)[ -> (w64 1074140560)])
          buffer_t value_2;
          state->vector_table_1074126664.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_2);
          // EP node  10628
          // BDD node 150:vector_return(vector:(w64 1074126664), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074140560)[(ReadLSB w16 (w32 0) vector_data__149)])
          // EP node  11315
          // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__149)))
          if ((65535) != ((u16)value_2.get(0, 2))) {
            // EP node  11316
            // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__149)))
            // EP node  11493
            // BDD node 155:FORWARD
            cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
          } else {
            // EP node  11317
            // BDD node 154:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__149)))
            // EP node  11494
            // BDD node 156:DROP
            result.forward = false;
          }
        } else {
          // EP node  8003
          // BDD node 148:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__147))
          // EP node  8293
          // BDD node 160:vector_borrow(vector:(w64 1074079320), index:(ReadLSB w32 (w32 0) new_index__147), val_out:(w64 1074250464)[ -> (w64 1074093216)])
          // EP node  8589
          // BDD node 163:vector_borrow(vector:(w64 1074126664), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074253392)[ -> (w64 1074140560)])
          buffer_t value_3;
          state->vector_table_1074126664.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_3);
          // EP node  8964
          // BDD node 158:map_put(map:(w64 1074048280), key:(w64 1074076000)[(ReadLSB w32 (w32 268) packet_chunks) -> (ReadLSB w32 (w32 268) packet_chunks)], value:(ReadLSB w32 (w32 0) new_index__147))
          buffer_t map_table_1074048280_key_1(4);
          map_table_1074048280_key_1[0] = *(u8*)(hdr_1 + 12);
          map_table_1074048280_key_1[1] = *(u8*)(hdr_1 + 13);
          map_table_1074048280_key_1[2] = *(u8*)(hdr_1 + 14);
          map_table_1074048280_key_1[3] = *(u8*)(hdr_1 + 15);
          state->map_table_1074048280.put(map_table_1074048280_key_1, allocated_index_0);
          // EP node  9344
          // BDD node 161:vector_return(vector:(w64 1074079320), index:(ReadLSB w32 (w32 0) new_index__147), value:(w64 1074093216)[(w32 1)])
          buffer_t vector_register_1074079320_value_0(4);
          vector_register_1074079320_value_0[0] = 0;
          vector_register_1074079320_value_0[1] = 0;
          vector_register_1074079320_value_0[2] = 0;
          vector_register_1074079320_value_0[3] = 1;
          state->vector_register_1074079320.put(allocated_index_0, vector_register_1074079320_value_0);
          // EP node  9499
          // BDD node 162:bf_set(bf:(w64 1074096872), key:(w64 1074250482)[(Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks))) -> (Concat w48 (Read w8 (w32 515) packet_chunks) (Concat w40 (Read w8 (w32 514) packet_chunks) (ReadLSB w32 (w32 268) packet_chunks)))])
          buffer_t bf_1074096872_key_0(6);
          bf_1074096872_key_0[0] = *(u8*)(hdr_1 + 12);
          bf_1074096872_key_0[1] = *(u8*)(hdr_1 + 13);
          bf_1074096872_key_0[2] = *(u8*)(hdr_1 + 14);
          bf_1074096872_key_0[3] = *(u8*)(hdr_1 + 15);
          bf_1074096872_key_0[4] = *(u8*)(hdr_2 + 2);
          bf_1074096872_key_0[5] = *(u8*)(hdr_2 + 3);
          state->bf_1074096872.set(bf_1074096872_key_0);
          // EP node  9655
          // BDD node 164:vector_return(vector:(w64 1074126664), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074140560)[(ReadLSB w16 (w32 0) vector_data__163)])
          // EP node  10129
          // BDD node 168:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__163)))
          if ((65535) != ((u16)value_3.get(0, 2))) {
            // EP node  10130
            // BDD node 168:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__163)))
            // EP node  10375
            // BDD node 169:FORWARD
            cpu_hdr->egress_dev = bswap16((u16)value_3.get(0, 2));
          } else {
            // EP node  10131
            // BDD node 168:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data__163)))
            // EP node  10713
            // BDD node 170:DROP
            result.forward = false;
          }
        }
      } else {
        // EP node  5673
        // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  5674
        // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  5668
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  5669
      // BDD node 147:dchain_allocate_new_index(chain:(w64 1074096456), index_out:(w64 1074250368)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__147)], time:(ReadLSB w64 (w32 0) next_time))
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
