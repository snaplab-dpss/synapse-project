#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  MapTable map_table_1074047904;
  DchainTable dchain_table_1074079888;
  CountMinSketch cms_1074080304;
  VectorTable vector_table_1074092880;
  VectorTable vector_table_1074110096;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      map_table_1074047904("map_table_1074047904",{"Ingress.map_table_1074047904_144",}, 100LL),
      dchain_table_1074079888("dchain_table_1074079888",{"Ingress.dchain_table_1074079888_174",}, 100LL),
      cms_1074080304("cms_1074080304",{"Ingress.cms_1074080304_row_0", "Ingress.cms_1074080304_row_1", "Ingress.cms_1074080304_row_2", "Ingress.cms_1074080304_row_3", }, 10LL),
      vector_table_1074092880("vector_table_1074092880",{"Ingress.vector_table_1074092880_141",}),
      vector_table_1074110096("vector_table_1074110096",{"Ingress.vector_table_1074110096_183","Ingress.vector_table_1074110096_175",})
    {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(3), 2);
  state->forward_nf_dev.add_entry(2, asic_get_dev_port(3));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(4), 3);
  state->forward_nf_dev.add_entry(3, asic_get_dev_port(4));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(5), 4);
  state->forward_nf_dev.add_entry(4, asic_get_dev_port(5));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(6), 5);
  state->forward_nf_dev.add_entry(5, asic_get_dev_port(6));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(7), 6);
  state->forward_nf_dev.add_entry(6, asic_get_dev_port(7));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(8), 7);
  state->forward_nf_dev.add_entry(7, asic_get_dev_port(8));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(9), 8);
  state->forward_nf_dev.add_entry(8, asic_get_dev_port(9));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(10), 9);
  state->forward_nf_dev.add_entry(9, asic_get_dev_port(10));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(11), 10);
  state->forward_nf_dev.add_entry(10, asic_get_dev_port(11));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(12), 11);
  state->forward_nf_dev.add_entry(11, asic_get_dev_port(12));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(13), 12);
  state->forward_nf_dev.add_entry(12, asic_get_dev_port(13));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(14), 13);
  state->forward_nf_dev.add_entry(13, asic_get_dev_port(14));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(15), 14);
  state->forward_nf_dev.add_entry(14, asic_get_dev_port(15));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(16), 15);
  state->forward_nf_dev.add_entry(15, asic_get_dev_port(16));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(17), 16);
  state->forward_nf_dev.add_entry(16, asic_get_dev_port(17));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(18), 17);
  state->forward_nf_dev.add_entry(17, asic_get_dev_port(18));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(19), 18);
  state->forward_nf_dev.add_entry(18, asic_get_dev_port(19));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(20), 19);
  state->forward_nf_dev.add_entry(19, asic_get_dev_port(20));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(21), 20);
  state->forward_nf_dev.add_entry(20, asic_get_dev_port(21));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(22), 21);
  state->forward_nf_dev.add_entry(21, asic_get_dev_port(22));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(23), 22);
  state->forward_nf_dev.add_entry(22, asic_get_dev_port(23));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(24), 23);
  state->forward_nf_dev.add_entry(23, asic_get_dev_port(24));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(25), 24);
  state->forward_nf_dev.add_entry(24, asic_get_dev_port(25));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(26), 25);
  state->forward_nf_dev.add_entry(25, asic_get_dev_port(26));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(27), 26);
  state->forward_nf_dev.add_entry(26, asic_get_dev_port(27));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(28), 27);
  state->forward_nf_dev.add_entry(27, asic_get_dev_port(28));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(29), 28);
  state->forward_nf_dev.add_entry(28, asic_get_dev_port(29));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(30), 29);
  state->forward_nf_dev.add_entry(29, asic_get_dev_port(30));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(31), 30);
  state->forward_nf_dev.add_entry(30, asic_get_dev_port(31));
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(32), 31);
  state->forward_nf_dev.add_entry(31, asic_get_dev_port(32));
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 13), map_out:(w64 1074047632)[(w64 0) -> (w64 1074047904)])
  // Module DataplaneMapTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074047648)[ -> (w64 1074079888)])
  // Module DataplaneDchainTableAllocate
  // BDD node 3:cms_allocate(height:(w32 4), width:(w32 1024), key_size:(w16 8), cms_out:(w64 1074047656)[(w64 0) -> (w64 1074080304)], cleanup_interval:(w64 10000000))
  // Module DataplaneCMSAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074047664)[(w64 0) -> (w64 1074092880)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074047672)[(w64 0) -> (w64 1074110096)])
  // Module DataplaneVectorTableAllocate
  // BDD node 6:vector_borrow(vector:(w64 1074092880), index:(w32 0), val_out:(w64 1074047504)[ -> (w64 1074106776)])
  // Module Ignore
  // BDD node 7:vector_return(vector:(w64 1074092880), index:(w32 0), value:(w64 1074106776)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_0(4);
  vector_table_1074092880_value_0[0] = 0;
  vector_table_1074092880_value_0[1] = 0;
  vector_table_1074092880_value_0[2] = 0;
  vector_table_1074092880_value_0[3] = 1;
  state->vector_table_1074092880.write(0, vector_table_1074092880_value_0);
  // BDD node 8:vector_borrow(vector:(w64 1074110096), index:(w32 0), val_out:(w64 1074047568)[ -> (w64 1074123992)])
  // Module Ignore
  // BDD node 9:vector_return(vector:(w64 1074110096), index:(w32 0), value:(w64 1074123992)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_0(2);
  vector_table_1074110096_value_0[0] = 0;
  vector_table_1074110096_value_0[1] = 1;
  state->vector_table_1074110096.write(0, vector_table_1074110096_value_0);
  // BDD node 10:vector_borrow(vector:(w64 1074092880), index:(w32 1), val_out:(w64 1074047504)[ -> (w64 1074106800)])
  // Module Ignore
  // BDD node 11:vector_return(vector:(w64 1074092880), index:(w32 1), value:(w64 1074106800)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_1(4);
  vector_table_1074092880_value_1[0] = 0;
  vector_table_1074092880_value_1[1] = 0;
  vector_table_1074092880_value_1[2] = 0;
  vector_table_1074092880_value_1[3] = 0;
  state->vector_table_1074092880.write(1, vector_table_1074092880_value_1);
  // BDD node 12:vector_borrow(vector:(w64 1074110096), index:(w32 1), val_out:(w64 1074047568)[ -> (w64 1074124016)])
  // Module Ignore
  // BDD node 13:vector_return(vector:(w64 1074110096), index:(w32 1), value:(w64 1074124016)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_1(2);
  vector_table_1074110096_value_1[0] = 0;
  vector_table_1074110096_value_1[1] = 0;
  state->vector_table_1074110096.write(1, vector_table_1074110096_value_1);
  // BDD node 14:vector_borrow(vector:(w64 1074092880), index:(w32 2), val_out:(w64 1074047504)[ -> (w64 1074106824)])
  // Module Ignore
  // BDD node 15:vector_return(vector:(w64 1074092880), index:(w32 2), value:(w64 1074106824)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_2(4);
  vector_table_1074092880_value_2[0] = 0;
  vector_table_1074092880_value_2[1] = 0;
  vector_table_1074092880_value_2[2] = 0;
  vector_table_1074092880_value_2[3] = 1;
  state->vector_table_1074092880.write(2, vector_table_1074092880_value_2);
  // BDD node 16:vector_borrow(vector:(w64 1074110096), index:(w32 2), val_out:(w64 1074047568)[ -> (w64 1074124040)])
  // Module Ignore
  // BDD node 17:vector_return(vector:(w64 1074110096), index:(w32 2), value:(w64 1074124040)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_2(2);
  vector_table_1074110096_value_2[0] = 0;
  vector_table_1074110096_value_2[1] = 3;
  state->vector_table_1074110096.write(2, vector_table_1074110096_value_2);
  // BDD node 18:vector_borrow(vector:(w64 1074092880), index:(w32 3), val_out:(w64 1074047504)[ -> (w64 1074106848)])
  // Module Ignore
  // BDD node 19:vector_return(vector:(w64 1074092880), index:(w32 3), value:(w64 1074106848)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_3(4);
  vector_table_1074092880_value_3[0] = 0;
  vector_table_1074092880_value_3[1] = 0;
  vector_table_1074092880_value_3[2] = 0;
  vector_table_1074092880_value_3[3] = 0;
  state->vector_table_1074092880.write(3, vector_table_1074092880_value_3);
  // BDD node 20:vector_borrow(vector:(w64 1074110096), index:(w32 3), val_out:(w64 1074047568)[ -> (w64 1074124064)])
  // Module Ignore
  // BDD node 21:vector_return(vector:(w64 1074110096), index:(w32 3), value:(w64 1074124064)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_3(2);
  vector_table_1074110096_value_3[0] = 0;
  vector_table_1074110096_value_3[1] = 2;
  state->vector_table_1074110096.write(3, vector_table_1074110096_value_3);
  // BDD node 22:vector_borrow(vector:(w64 1074092880), index:(w32 4), val_out:(w64 1074047504)[ -> (w64 1074106872)])
  // Module Ignore
  // BDD node 23:vector_return(vector:(w64 1074092880), index:(w32 4), value:(w64 1074106872)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_4(4);
  vector_table_1074092880_value_4[0] = 0;
  vector_table_1074092880_value_4[1] = 0;
  vector_table_1074092880_value_4[2] = 0;
  vector_table_1074092880_value_4[3] = 1;
  state->vector_table_1074092880.write(4, vector_table_1074092880_value_4);
  // BDD node 24:vector_borrow(vector:(w64 1074110096), index:(w32 4), val_out:(w64 1074047568)[ -> (w64 1074124088)])
  // Module Ignore
  // BDD node 25:vector_return(vector:(w64 1074110096), index:(w32 4), value:(w64 1074124088)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_4(2);
  vector_table_1074110096_value_4[0] = 0;
  vector_table_1074110096_value_4[1] = 5;
  state->vector_table_1074110096.write(4, vector_table_1074110096_value_4);
  // BDD node 26:vector_borrow(vector:(w64 1074092880), index:(w32 5), val_out:(w64 1074047504)[ -> (w64 1074106896)])
  // Module Ignore
  // BDD node 27:vector_return(vector:(w64 1074092880), index:(w32 5), value:(w64 1074106896)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_5(4);
  vector_table_1074092880_value_5[0] = 0;
  vector_table_1074092880_value_5[1] = 0;
  vector_table_1074092880_value_5[2] = 0;
  vector_table_1074092880_value_5[3] = 0;
  state->vector_table_1074092880.write(5, vector_table_1074092880_value_5);
  // BDD node 28:vector_borrow(vector:(w64 1074110096), index:(w32 5), val_out:(w64 1074047568)[ -> (w64 1074124112)])
  // Module Ignore
  // BDD node 29:vector_return(vector:(w64 1074110096), index:(w32 5), value:(w64 1074124112)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_5(2);
  vector_table_1074110096_value_5[0] = 0;
  vector_table_1074110096_value_5[1] = 4;
  state->vector_table_1074110096.write(5, vector_table_1074110096_value_5);
  // BDD node 30:vector_borrow(vector:(w64 1074092880), index:(w32 6), val_out:(w64 1074047504)[ -> (w64 1074106920)])
  // Module Ignore
  // BDD node 31:vector_return(vector:(w64 1074092880), index:(w32 6), value:(w64 1074106920)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_6(4);
  vector_table_1074092880_value_6[0] = 0;
  vector_table_1074092880_value_6[1] = 0;
  vector_table_1074092880_value_6[2] = 0;
  vector_table_1074092880_value_6[3] = 1;
  state->vector_table_1074092880.write(6, vector_table_1074092880_value_6);
  // BDD node 32:vector_borrow(vector:(w64 1074110096), index:(w32 6), val_out:(w64 1074047568)[ -> (w64 1074124136)])
  // Module Ignore
  // BDD node 33:vector_return(vector:(w64 1074110096), index:(w32 6), value:(w64 1074124136)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_6(2);
  vector_table_1074110096_value_6[0] = 0;
  vector_table_1074110096_value_6[1] = 7;
  state->vector_table_1074110096.write(6, vector_table_1074110096_value_6);
  // BDD node 34:vector_borrow(vector:(w64 1074092880), index:(w32 7), val_out:(w64 1074047504)[ -> (w64 1074106944)])
  // Module Ignore
  // BDD node 35:vector_return(vector:(w64 1074092880), index:(w32 7), value:(w64 1074106944)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_7(4);
  vector_table_1074092880_value_7[0] = 0;
  vector_table_1074092880_value_7[1] = 0;
  vector_table_1074092880_value_7[2] = 0;
  vector_table_1074092880_value_7[3] = 0;
  state->vector_table_1074092880.write(7, vector_table_1074092880_value_7);
  // BDD node 36:vector_borrow(vector:(w64 1074110096), index:(w32 7), val_out:(w64 1074047568)[ -> (w64 1074124160)])
  // Module Ignore
  // BDD node 37:vector_return(vector:(w64 1074110096), index:(w32 7), value:(w64 1074124160)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_7(2);
  vector_table_1074110096_value_7[0] = 0;
  vector_table_1074110096_value_7[1] = 6;
  state->vector_table_1074110096.write(7, vector_table_1074110096_value_7);
  // BDD node 38:vector_borrow(vector:(w64 1074092880), index:(w32 8), val_out:(w64 1074047504)[ -> (w64 1074106968)])
  // Module Ignore
  // BDD node 39:vector_return(vector:(w64 1074092880), index:(w32 8), value:(w64 1074106968)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_8(4);
  vector_table_1074092880_value_8[0] = 0;
  vector_table_1074092880_value_8[1] = 0;
  vector_table_1074092880_value_8[2] = 0;
  vector_table_1074092880_value_8[3] = 1;
  state->vector_table_1074092880.write(8, vector_table_1074092880_value_8);
  // BDD node 40:vector_borrow(vector:(w64 1074110096), index:(w32 8), val_out:(w64 1074047568)[ -> (w64 1074124184)])
  // Module Ignore
  // BDD node 41:vector_return(vector:(w64 1074110096), index:(w32 8), value:(w64 1074124184)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_8(2);
  vector_table_1074110096_value_8[0] = 0;
  vector_table_1074110096_value_8[1] = 9;
  state->vector_table_1074110096.write(8, vector_table_1074110096_value_8);
  // BDD node 42:vector_borrow(vector:(w64 1074092880), index:(w32 9), val_out:(w64 1074047504)[ -> (w64 1074106992)])
  // Module Ignore
  // BDD node 43:vector_return(vector:(w64 1074092880), index:(w32 9), value:(w64 1074106992)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_9(4);
  vector_table_1074092880_value_9[0] = 0;
  vector_table_1074092880_value_9[1] = 0;
  vector_table_1074092880_value_9[2] = 0;
  vector_table_1074092880_value_9[3] = 0;
  state->vector_table_1074092880.write(9, vector_table_1074092880_value_9);
  // BDD node 44:vector_borrow(vector:(w64 1074110096), index:(w32 9), val_out:(w64 1074047568)[ -> (w64 1074124208)])
  // Module Ignore
  // BDD node 45:vector_return(vector:(w64 1074110096), index:(w32 9), value:(w64 1074124208)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_9(2);
  vector_table_1074110096_value_9[0] = 0;
  vector_table_1074110096_value_9[1] = 8;
  state->vector_table_1074110096.write(9, vector_table_1074110096_value_9);
  // BDD node 46:vector_borrow(vector:(w64 1074092880), index:(w32 10), val_out:(w64 1074047504)[ -> (w64 1074107016)])
  // Module Ignore
  // BDD node 47:vector_return(vector:(w64 1074092880), index:(w32 10), value:(w64 1074107016)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_10(4);
  vector_table_1074092880_value_10[0] = 0;
  vector_table_1074092880_value_10[1] = 0;
  vector_table_1074092880_value_10[2] = 0;
  vector_table_1074092880_value_10[3] = 1;
  state->vector_table_1074092880.write(10, vector_table_1074092880_value_10);
  // BDD node 48:vector_borrow(vector:(w64 1074110096), index:(w32 10), val_out:(w64 1074047568)[ -> (w64 1074124232)])
  // Module Ignore
  // BDD node 49:vector_return(vector:(w64 1074110096), index:(w32 10), value:(w64 1074124232)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_10(2);
  vector_table_1074110096_value_10[0] = 0;
  vector_table_1074110096_value_10[1] = 11;
  state->vector_table_1074110096.write(10, vector_table_1074110096_value_10);
  // BDD node 50:vector_borrow(vector:(w64 1074092880), index:(w32 11), val_out:(w64 1074047504)[ -> (w64 1074107040)])
  // Module Ignore
  // BDD node 51:vector_return(vector:(w64 1074092880), index:(w32 11), value:(w64 1074107040)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_11(4);
  vector_table_1074092880_value_11[0] = 0;
  vector_table_1074092880_value_11[1] = 0;
  vector_table_1074092880_value_11[2] = 0;
  vector_table_1074092880_value_11[3] = 0;
  state->vector_table_1074092880.write(11, vector_table_1074092880_value_11);
  // BDD node 52:vector_borrow(vector:(w64 1074110096), index:(w32 11), val_out:(w64 1074047568)[ -> (w64 1074124256)])
  // Module Ignore
  // BDD node 53:vector_return(vector:(w64 1074110096), index:(w32 11), value:(w64 1074124256)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_11(2);
  vector_table_1074110096_value_11[0] = 0;
  vector_table_1074110096_value_11[1] = 10;
  state->vector_table_1074110096.write(11, vector_table_1074110096_value_11);
  // BDD node 54:vector_borrow(vector:(w64 1074092880), index:(w32 12), val_out:(w64 1074047504)[ -> (w64 1074107064)])
  // Module Ignore
  // BDD node 55:vector_return(vector:(w64 1074092880), index:(w32 12), value:(w64 1074107064)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_12(4);
  vector_table_1074092880_value_12[0] = 0;
  vector_table_1074092880_value_12[1] = 0;
  vector_table_1074092880_value_12[2] = 0;
  vector_table_1074092880_value_12[3] = 1;
  state->vector_table_1074092880.write(12, vector_table_1074092880_value_12);
  // BDD node 56:vector_borrow(vector:(w64 1074110096), index:(w32 12), val_out:(w64 1074047568)[ -> (w64 1074124280)])
  // Module Ignore
  // BDD node 57:vector_return(vector:(w64 1074110096), index:(w32 12), value:(w64 1074124280)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_12(2);
  vector_table_1074110096_value_12[0] = 0;
  vector_table_1074110096_value_12[1] = 13;
  state->vector_table_1074110096.write(12, vector_table_1074110096_value_12);
  // BDD node 58:vector_borrow(vector:(w64 1074092880), index:(w32 13), val_out:(w64 1074047504)[ -> (w64 1074107088)])
  // Module Ignore
  // BDD node 59:vector_return(vector:(w64 1074092880), index:(w32 13), value:(w64 1074107088)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_13(4);
  vector_table_1074092880_value_13[0] = 0;
  vector_table_1074092880_value_13[1] = 0;
  vector_table_1074092880_value_13[2] = 0;
  vector_table_1074092880_value_13[3] = 0;
  state->vector_table_1074092880.write(13, vector_table_1074092880_value_13);
  // BDD node 60:vector_borrow(vector:(w64 1074110096), index:(w32 13), val_out:(w64 1074047568)[ -> (w64 1074124304)])
  // Module Ignore
  // BDD node 61:vector_return(vector:(w64 1074110096), index:(w32 13), value:(w64 1074124304)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_13(2);
  vector_table_1074110096_value_13[0] = 0;
  vector_table_1074110096_value_13[1] = 12;
  state->vector_table_1074110096.write(13, vector_table_1074110096_value_13);
  // BDD node 62:vector_borrow(vector:(w64 1074092880), index:(w32 14), val_out:(w64 1074047504)[ -> (w64 1074107112)])
  // Module Ignore
  // BDD node 63:vector_return(vector:(w64 1074092880), index:(w32 14), value:(w64 1074107112)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_14(4);
  vector_table_1074092880_value_14[0] = 0;
  vector_table_1074092880_value_14[1] = 0;
  vector_table_1074092880_value_14[2] = 0;
  vector_table_1074092880_value_14[3] = 1;
  state->vector_table_1074092880.write(14, vector_table_1074092880_value_14);
  // BDD node 64:vector_borrow(vector:(w64 1074110096), index:(w32 14), val_out:(w64 1074047568)[ -> (w64 1074124328)])
  // Module Ignore
  // BDD node 65:vector_return(vector:(w64 1074110096), index:(w32 14), value:(w64 1074124328)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_14(2);
  vector_table_1074110096_value_14[0] = 0;
  vector_table_1074110096_value_14[1] = 15;
  state->vector_table_1074110096.write(14, vector_table_1074110096_value_14);
  // BDD node 66:vector_borrow(vector:(w64 1074092880), index:(w32 15), val_out:(w64 1074047504)[ -> (w64 1074107136)])
  // Module Ignore
  // BDD node 67:vector_return(vector:(w64 1074092880), index:(w32 15), value:(w64 1074107136)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_15(4);
  vector_table_1074092880_value_15[0] = 0;
  vector_table_1074092880_value_15[1] = 0;
  vector_table_1074092880_value_15[2] = 0;
  vector_table_1074092880_value_15[3] = 0;
  state->vector_table_1074092880.write(15, vector_table_1074092880_value_15);
  // BDD node 68:vector_borrow(vector:(w64 1074110096), index:(w32 15), val_out:(w64 1074047568)[ -> (w64 1074124352)])
  // Module Ignore
  // BDD node 69:vector_return(vector:(w64 1074110096), index:(w32 15), value:(w64 1074124352)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_15(2);
  vector_table_1074110096_value_15[0] = 0;
  vector_table_1074110096_value_15[1] = 14;
  state->vector_table_1074110096.write(15, vector_table_1074110096_value_15);
  // BDD node 70:vector_borrow(vector:(w64 1074092880), index:(w32 16), val_out:(w64 1074047504)[ -> (w64 1074107160)])
  // Module Ignore
  // BDD node 71:vector_return(vector:(w64 1074092880), index:(w32 16), value:(w64 1074107160)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_16(4);
  vector_table_1074092880_value_16[0] = 0;
  vector_table_1074092880_value_16[1] = 0;
  vector_table_1074092880_value_16[2] = 0;
  vector_table_1074092880_value_16[3] = 1;
  state->vector_table_1074092880.write(16, vector_table_1074092880_value_16);
  // BDD node 72:vector_borrow(vector:(w64 1074110096), index:(w32 16), val_out:(w64 1074047568)[ -> (w64 1074124376)])
  // Module Ignore
  // BDD node 73:vector_return(vector:(w64 1074110096), index:(w32 16), value:(w64 1074124376)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_16(2);
  vector_table_1074110096_value_16[0] = 0;
  vector_table_1074110096_value_16[1] = 17;
  state->vector_table_1074110096.write(16, vector_table_1074110096_value_16);
  // BDD node 74:vector_borrow(vector:(w64 1074092880), index:(w32 17), val_out:(w64 1074047504)[ -> (w64 1074107184)])
  // Module Ignore
  // BDD node 75:vector_return(vector:(w64 1074092880), index:(w32 17), value:(w64 1074107184)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_17(4);
  vector_table_1074092880_value_17[0] = 0;
  vector_table_1074092880_value_17[1] = 0;
  vector_table_1074092880_value_17[2] = 0;
  vector_table_1074092880_value_17[3] = 0;
  state->vector_table_1074092880.write(17, vector_table_1074092880_value_17);
  // BDD node 76:vector_borrow(vector:(w64 1074110096), index:(w32 17), val_out:(w64 1074047568)[ -> (w64 1074124400)])
  // Module Ignore
  // BDD node 77:vector_return(vector:(w64 1074110096), index:(w32 17), value:(w64 1074124400)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_17(2);
  vector_table_1074110096_value_17[0] = 0;
  vector_table_1074110096_value_17[1] = 16;
  state->vector_table_1074110096.write(17, vector_table_1074110096_value_17);
  // BDD node 78:vector_borrow(vector:(w64 1074092880), index:(w32 18), val_out:(w64 1074047504)[ -> (w64 1074107208)])
  // Module Ignore
  // BDD node 79:vector_return(vector:(w64 1074092880), index:(w32 18), value:(w64 1074107208)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_18(4);
  vector_table_1074092880_value_18[0] = 0;
  vector_table_1074092880_value_18[1] = 0;
  vector_table_1074092880_value_18[2] = 0;
  vector_table_1074092880_value_18[3] = 1;
  state->vector_table_1074092880.write(18, vector_table_1074092880_value_18);
  // BDD node 80:vector_borrow(vector:(w64 1074110096), index:(w32 18), val_out:(w64 1074047568)[ -> (w64 1074124424)])
  // Module Ignore
  // BDD node 81:vector_return(vector:(w64 1074110096), index:(w32 18), value:(w64 1074124424)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_18(2);
  vector_table_1074110096_value_18[0] = 0;
  vector_table_1074110096_value_18[1] = 19;
  state->vector_table_1074110096.write(18, vector_table_1074110096_value_18);
  // BDD node 82:vector_borrow(vector:(w64 1074092880), index:(w32 19), val_out:(w64 1074047504)[ -> (w64 1074107232)])
  // Module Ignore
  // BDD node 83:vector_return(vector:(w64 1074092880), index:(w32 19), value:(w64 1074107232)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_19(4);
  vector_table_1074092880_value_19[0] = 0;
  vector_table_1074092880_value_19[1] = 0;
  vector_table_1074092880_value_19[2] = 0;
  vector_table_1074092880_value_19[3] = 0;
  state->vector_table_1074092880.write(19, vector_table_1074092880_value_19);
  // BDD node 84:vector_borrow(vector:(w64 1074110096), index:(w32 19), val_out:(w64 1074047568)[ -> (w64 1074124448)])
  // Module Ignore
  // BDD node 85:vector_return(vector:(w64 1074110096), index:(w32 19), value:(w64 1074124448)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_19(2);
  vector_table_1074110096_value_19[0] = 0;
  vector_table_1074110096_value_19[1] = 18;
  state->vector_table_1074110096.write(19, vector_table_1074110096_value_19);
  // BDD node 86:vector_borrow(vector:(w64 1074092880), index:(w32 20), val_out:(w64 1074047504)[ -> (w64 1074107256)])
  // Module Ignore
  // BDD node 87:vector_return(vector:(w64 1074092880), index:(w32 20), value:(w64 1074107256)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_20(4);
  vector_table_1074092880_value_20[0] = 0;
  vector_table_1074092880_value_20[1] = 0;
  vector_table_1074092880_value_20[2] = 0;
  vector_table_1074092880_value_20[3] = 1;
  state->vector_table_1074092880.write(20, vector_table_1074092880_value_20);
  // BDD node 88:vector_borrow(vector:(w64 1074110096), index:(w32 20), val_out:(w64 1074047568)[ -> (w64 1074124472)])
  // Module Ignore
  // BDD node 89:vector_return(vector:(w64 1074110096), index:(w32 20), value:(w64 1074124472)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_20(2);
  vector_table_1074110096_value_20[0] = 0;
  vector_table_1074110096_value_20[1] = 21;
  state->vector_table_1074110096.write(20, vector_table_1074110096_value_20);
  // BDD node 90:vector_borrow(vector:(w64 1074092880), index:(w32 21), val_out:(w64 1074047504)[ -> (w64 1074107280)])
  // Module Ignore
  // BDD node 91:vector_return(vector:(w64 1074092880), index:(w32 21), value:(w64 1074107280)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_21(4);
  vector_table_1074092880_value_21[0] = 0;
  vector_table_1074092880_value_21[1] = 0;
  vector_table_1074092880_value_21[2] = 0;
  vector_table_1074092880_value_21[3] = 0;
  state->vector_table_1074092880.write(21, vector_table_1074092880_value_21);
  // BDD node 92:vector_borrow(vector:(w64 1074110096), index:(w32 21), val_out:(w64 1074047568)[ -> (w64 1074124496)])
  // Module Ignore
  // BDD node 93:vector_return(vector:(w64 1074110096), index:(w32 21), value:(w64 1074124496)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_21(2);
  vector_table_1074110096_value_21[0] = 0;
  vector_table_1074110096_value_21[1] = 20;
  state->vector_table_1074110096.write(21, vector_table_1074110096_value_21);
  // BDD node 94:vector_borrow(vector:(w64 1074092880), index:(w32 22), val_out:(w64 1074047504)[ -> (w64 1074107304)])
  // Module Ignore
  // BDD node 95:vector_return(vector:(w64 1074092880), index:(w32 22), value:(w64 1074107304)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_22(4);
  vector_table_1074092880_value_22[0] = 0;
  vector_table_1074092880_value_22[1] = 0;
  vector_table_1074092880_value_22[2] = 0;
  vector_table_1074092880_value_22[3] = 1;
  state->vector_table_1074092880.write(22, vector_table_1074092880_value_22);
  // BDD node 96:vector_borrow(vector:(w64 1074110096), index:(w32 22), val_out:(w64 1074047568)[ -> (w64 1074124520)])
  // Module Ignore
  // BDD node 97:vector_return(vector:(w64 1074110096), index:(w32 22), value:(w64 1074124520)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_22(2);
  vector_table_1074110096_value_22[0] = 0;
  vector_table_1074110096_value_22[1] = 23;
  state->vector_table_1074110096.write(22, vector_table_1074110096_value_22);
  // BDD node 98:vector_borrow(vector:(w64 1074092880), index:(w32 23), val_out:(w64 1074047504)[ -> (w64 1074107328)])
  // Module Ignore
  // BDD node 99:vector_return(vector:(w64 1074092880), index:(w32 23), value:(w64 1074107328)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_23(4);
  vector_table_1074092880_value_23[0] = 0;
  vector_table_1074092880_value_23[1] = 0;
  vector_table_1074092880_value_23[2] = 0;
  vector_table_1074092880_value_23[3] = 0;
  state->vector_table_1074092880.write(23, vector_table_1074092880_value_23);
  // BDD node 100:vector_borrow(vector:(w64 1074110096), index:(w32 23), val_out:(w64 1074047568)[ -> (w64 1074124544)])
  // Module Ignore
  // BDD node 101:vector_return(vector:(w64 1074110096), index:(w32 23), value:(w64 1074124544)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_23(2);
  vector_table_1074110096_value_23[0] = 0;
  vector_table_1074110096_value_23[1] = 22;
  state->vector_table_1074110096.write(23, vector_table_1074110096_value_23);
  // BDD node 102:vector_borrow(vector:(w64 1074092880), index:(w32 24), val_out:(w64 1074047504)[ -> (w64 1074107352)])
  // Module Ignore
  // BDD node 103:vector_return(vector:(w64 1074092880), index:(w32 24), value:(w64 1074107352)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_24(4);
  vector_table_1074092880_value_24[0] = 0;
  vector_table_1074092880_value_24[1] = 0;
  vector_table_1074092880_value_24[2] = 0;
  vector_table_1074092880_value_24[3] = 1;
  state->vector_table_1074092880.write(24, vector_table_1074092880_value_24);
  // BDD node 104:vector_borrow(vector:(w64 1074110096), index:(w32 24), val_out:(w64 1074047568)[ -> (w64 1074124568)])
  // Module Ignore
  // BDD node 105:vector_return(vector:(w64 1074110096), index:(w32 24), value:(w64 1074124568)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_24(2);
  vector_table_1074110096_value_24[0] = 0;
  vector_table_1074110096_value_24[1] = 25;
  state->vector_table_1074110096.write(24, vector_table_1074110096_value_24);
  // BDD node 106:vector_borrow(vector:(w64 1074092880), index:(w32 25), val_out:(w64 1074047504)[ -> (w64 1074107376)])
  // Module Ignore
  // BDD node 107:vector_return(vector:(w64 1074092880), index:(w32 25), value:(w64 1074107376)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_25(4);
  vector_table_1074092880_value_25[0] = 0;
  vector_table_1074092880_value_25[1] = 0;
  vector_table_1074092880_value_25[2] = 0;
  vector_table_1074092880_value_25[3] = 0;
  state->vector_table_1074092880.write(25, vector_table_1074092880_value_25);
  // BDD node 108:vector_borrow(vector:(w64 1074110096), index:(w32 25), val_out:(w64 1074047568)[ -> (w64 1074124592)])
  // Module Ignore
  // BDD node 109:vector_return(vector:(w64 1074110096), index:(w32 25), value:(w64 1074124592)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_25(2);
  vector_table_1074110096_value_25[0] = 0;
  vector_table_1074110096_value_25[1] = 24;
  state->vector_table_1074110096.write(25, vector_table_1074110096_value_25);
  // BDD node 110:vector_borrow(vector:(w64 1074092880), index:(w32 26), val_out:(w64 1074047504)[ -> (w64 1074107400)])
  // Module Ignore
  // BDD node 111:vector_return(vector:(w64 1074092880), index:(w32 26), value:(w64 1074107400)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_26(4);
  vector_table_1074092880_value_26[0] = 0;
  vector_table_1074092880_value_26[1] = 0;
  vector_table_1074092880_value_26[2] = 0;
  vector_table_1074092880_value_26[3] = 1;
  state->vector_table_1074092880.write(26, vector_table_1074092880_value_26);
  // BDD node 112:vector_borrow(vector:(w64 1074110096), index:(w32 26), val_out:(w64 1074047568)[ -> (w64 1074124616)])
  // Module Ignore
  // BDD node 113:vector_return(vector:(w64 1074110096), index:(w32 26), value:(w64 1074124616)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_26(2);
  vector_table_1074110096_value_26[0] = 0;
  vector_table_1074110096_value_26[1] = 27;
  state->vector_table_1074110096.write(26, vector_table_1074110096_value_26);
  // BDD node 114:vector_borrow(vector:(w64 1074092880), index:(w32 27), val_out:(w64 1074047504)[ -> (w64 1074107424)])
  // Module Ignore
  // BDD node 115:vector_return(vector:(w64 1074092880), index:(w32 27), value:(w64 1074107424)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_27(4);
  vector_table_1074092880_value_27[0] = 0;
  vector_table_1074092880_value_27[1] = 0;
  vector_table_1074092880_value_27[2] = 0;
  vector_table_1074092880_value_27[3] = 0;
  state->vector_table_1074092880.write(27, vector_table_1074092880_value_27);
  // BDD node 116:vector_borrow(vector:(w64 1074110096), index:(w32 27), val_out:(w64 1074047568)[ -> (w64 1074124640)])
  // Module Ignore
  // BDD node 117:vector_return(vector:(w64 1074110096), index:(w32 27), value:(w64 1074124640)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_27(2);
  vector_table_1074110096_value_27[0] = 0;
  vector_table_1074110096_value_27[1] = 26;
  state->vector_table_1074110096.write(27, vector_table_1074110096_value_27);
  // BDD node 118:vector_borrow(vector:(w64 1074092880), index:(w32 28), val_out:(w64 1074047504)[ -> (w64 1074107448)])
  // Module Ignore
  // BDD node 119:vector_return(vector:(w64 1074092880), index:(w32 28), value:(w64 1074107448)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_28(4);
  vector_table_1074092880_value_28[0] = 0;
  vector_table_1074092880_value_28[1] = 0;
  vector_table_1074092880_value_28[2] = 0;
  vector_table_1074092880_value_28[3] = 1;
  state->vector_table_1074092880.write(28, vector_table_1074092880_value_28);
  // BDD node 120:vector_borrow(vector:(w64 1074110096), index:(w32 28), val_out:(w64 1074047568)[ -> (w64 1074124664)])
  // Module Ignore
  // BDD node 121:vector_return(vector:(w64 1074110096), index:(w32 28), value:(w64 1074124664)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_28(2);
  vector_table_1074110096_value_28[0] = 0;
  vector_table_1074110096_value_28[1] = 29;
  state->vector_table_1074110096.write(28, vector_table_1074110096_value_28);
  // BDD node 122:vector_borrow(vector:(w64 1074092880), index:(w32 29), val_out:(w64 1074047504)[ -> (w64 1074107472)])
  // Module Ignore
  // BDD node 123:vector_return(vector:(w64 1074092880), index:(w32 29), value:(w64 1074107472)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_29(4);
  vector_table_1074092880_value_29[0] = 0;
  vector_table_1074092880_value_29[1] = 0;
  vector_table_1074092880_value_29[2] = 0;
  vector_table_1074092880_value_29[3] = 0;
  state->vector_table_1074092880.write(29, vector_table_1074092880_value_29);
  // BDD node 124:vector_borrow(vector:(w64 1074110096), index:(w32 29), val_out:(w64 1074047568)[ -> (w64 1074124688)])
  // Module Ignore
  // BDD node 125:vector_return(vector:(w64 1074110096), index:(w32 29), value:(w64 1074124688)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_29(2);
  vector_table_1074110096_value_29[0] = 0;
  vector_table_1074110096_value_29[1] = 28;
  state->vector_table_1074110096.write(29, vector_table_1074110096_value_29);
  // BDD node 126:vector_borrow(vector:(w64 1074092880), index:(w32 30), val_out:(w64 1074047504)[ -> (w64 1074107496)])
  // Module Ignore
  // BDD node 127:vector_return(vector:(w64 1074092880), index:(w32 30), value:(w64 1074107496)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_30(4);
  vector_table_1074092880_value_30[0] = 0;
  vector_table_1074092880_value_30[1] = 0;
  vector_table_1074092880_value_30[2] = 0;
  vector_table_1074092880_value_30[3] = 1;
  state->vector_table_1074092880.write(30, vector_table_1074092880_value_30);
  // BDD node 128:vector_borrow(vector:(w64 1074110096), index:(w32 30), val_out:(w64 1074047568)[ -> (w64 1074124712)])
  // Module Ignore
  // BDD node 129:vector_return(vector:(w64 1074110096), index:(w32 30), value:(w64 1074124712)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_30(2);
  vector_table_1074110096_value_30[0] = 0;
  vector_table_1074110096_value_30[1] = 31;
  state->vector_table_1074110096.write(30, vector_table_1074110096_value_30);
  // BDD node 130:vector_borrow(vector:(w64 1074092880), index:(w32 31), val_out:(w64 1074047504)[ -> (w64 1074107520)])
  // Module Ignore
  // BDD node 131:vector_return(vector:(w64 1074092880), index:(w32 31), value:(w64 1074107520)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092880_value_31(4);
  vector_table_1074092880_value_31[0] = 0;
  vector_table_1074092880_value_31[1] = 0;
  vector_table_1074092880_value_31[2] = 0;
  vector_table_1074092880_value_31[3] = 0;
  state->vector_table_1074092880.write(31, vector_table_1074092880_value_31);
  // BDD node 132:vector_borrow(vector:(w64 1074110096), index:(w32 31), val_out:(w64 1074047568)[ -> (w64 1074124736)])
  // Module Ignore
  // BDD node 133:vector_return(vector:(w64 1074110096), index:(w32 31), value:(w64 1074124736)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110096_value_31(2);
  vector_table_1074110096_value_31[0] = 0;
  vector_table_1074110096_value_31[1] = 30;
  state->vector_table_1074110096.write(31, vector_table_1074110096_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 vector_data_512;
  u32 map_has_this_key;
  u32 min_estimate;
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

  if (bswap16(cpu_hdr->code_path) == 4450) {
    // EP node  4433
    // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  4434
    // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  4435
    // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  4436
    // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t value_0;
    state->vector_table_1074092880.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  4437
    // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  4438
      // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  4441
      // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      buffer_t map_table_1074047904_key_0(13);
      map_table_1074047904_key_0[0] = *(u8*)(hdr_2 + 0);
      map_table_1074047904_key_0[1] = *(u8*)(hdr_2 + 1);
      map_table_1074047904_key_0[2] = *(u8*)(hdr_2 + 2);
      map_table_1074047904_key_0[3] = *(u8*)(hdr_2 + 3);
      map_table_1074047904_key_0[4] = *(u8*)(hdr_1 + 12);
      map_table_1074047904_key_0[5] = *(u8*)(hdr_1 + 13);
      map_table_1074047904_key_0[6] = *(u8*)(hdr_1 + 14);
      map_table_1074047904_key_0[7] = *(u8*)(hdr_1 + 15);
      map_table_1074047904_key_0[8] = *(u8*)(hdr_1 + 16);
      map_table_1074047904_key_0[9] = *(u8*)(hdr_1 + 17);
      map_table_1074047904_key_0[10] = *(u8*)(hdr_1 + 18);
      map_table_1074047904_key_0[11] = *(u8*)(hdr_1 + 19);
      map_table_1074047904_key_0[12] = *(u8*)(hdr_1 + 9);
      u32 value_1;
      bool found_0 = state->map_table_1074047904.get(map_table_1074047904_key_0, value_1);
      // EP node  4442
      // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_0)) {
        // EP node  4443
        // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  4446
        // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        if ((bswap32(cpu_hdr_extra->min_estimate)) <= (64)) {
          // EP node  4447
          // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          // EP node  9784
          // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          u32 allocated_index_0;
          bool success_0 = state->dchain_table_1074079888.allocate_new_index(allocated_index_0);
          // EP node  9848
          // BDD node 150:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
          if ((0) == (success_0)) {
            // EP node  9849
            // BDD node 150:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
            // EP node  10828
            // BDD node 151:vector_borrow(vector:(w64 1074110096), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074226576)[ -> (w64 1074123992)])
            buffer_t value_2;
            state->vector_table_1074110096.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_2);
            // EP node  10978
            // BDD node 152:vector_return(vector:(w64 1074110096), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074123992)[(ReadLSB w16 (w32 0) vector_data_640)])
            // EP node  11510
            // BDD node 156:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
            if ((65535) != ((u16)value_2.get(0, 2))) {
              // EP node  11511
              // BDD node 156:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  11911
              // BDD node 157:FORWARD
              cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
            } else {
              // EP node  11512
              // BDD node 156:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  11589
              // BDD node 158:DROP
              result.forward = false;
            }
          } else {
            // EP node  9850
            // BDD node 150:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
            // EP node  9982
            // BDD node 160:map_put(map:(w64 1074047904), key:(w64 1074075624)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value:(ReadLSB w32 (w32 0) new_index))
            buffer_t map_table_1074047904_key_1(13);
            map_table_1074047904_key_1[0] = *(u8*)(hdr_2 + 0);
            map_table_1074047904_key_1[1] = *(u8*)(hdr_2 + 1);
            map_table_1074047904_key_1[2] = *(u8*)(hdr_2 + 2);
            map_table_1074047904_key_1[3] = *(u8*)(hdr_2 + 3);
            map_table_1074047904_key_1[4] = *(u8*)(hdr_1 + 12);
            map_table_1074047904_key_1[5] = *(u8*)(hdr_1 + 13);
            map_table_1074047904_key_1[6] = *(u8*)(hdr_1 + 14);
            map_table_1074047904_key_1[7] = *(u8*)(hdr_1 + 15);
            map_table_1074047904_key_1[8] = *(u8*)(hdr_1 + 16);
            map_table_1074047904_key_1[9] = *(u8*)(hdr_1 + 17);
            map_table_1074047904_key_1[10] = *(u8*)(hdr_1 + 18);
            map_table_1074047904_key_1[11] = *(u8*)(hdr_1 + 19);
            map_table_1074047904_key_1[12] = *(u8*)(hdr_1 + 9);
            state->map_table_1074047904.put(map_table_1074047904_key_1, allocated_index_0);
            // EP node  10050
            // BDD node 162:vector_borrow(vector:(w64 1074110096), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074228808)[ -> (w64 1074123992)])
            buffer_t value_3;
            state->vector_table_1074110096.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_3);
            // EP node  10188
            // BDD node 163:vector_return(vector:(w64 1074110096), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074123992)[(ReadLSB w16 (w32 0) vector_data_640)])
            // EP node  10608
            // BDD node 167:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
            if ((65535) != ((u16)value_3.get(0, 2))) {
              // EP node  10609
              // BDD node 167:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  11829
              // BDD node 168:FORWARD
              cpu_hdr->egress_dev = bswap16((u16)value_3.get(0, 2));
            } else {
              // EP node  10610
              // BDD node 167:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_640)))
              // EP node  10754
              // BDD node 169:DROP
              result.forward = false;
            }
          }
        } else {
          // EP node  4448
          // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          // EP node  4449
          // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          result.abort_transaction = true;
          cpu_hdr->trigger_dataplane_execution = 1;
          return result;
        }
      } else {
        // EP node  4444
        // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  4445
        // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  4439
      // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  4440
      // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079888), index_out:(w64 1074226376)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
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
