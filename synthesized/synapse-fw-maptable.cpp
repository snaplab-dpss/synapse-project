#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  MapTable map_table_1074044016;
  DchainTable dchain_table_1074076000;
  VectorTable vector_table_1074076424;
  VectorTable vector_table_1074093640;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      map_table_1074044016("map_table_1074044016",{"Ingress.map_table_1074044016_157","Ingress.map_table_1074044016_142",}, 1000LL),
      dchain_table_1074076000("dchain_table_1074076000",{"Ingress.dchain_table_1074076000_180",}, 1000LL),
      vector_table_1074076424("vector_table_1074076424",{"Ingress.vector_table_1074076424_139",}),
      vector_table_1074093640("vector_table_1074093640",{"Ingress.vector_table_1074093640_149",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 13), map_out:(w64 1074043752)[(w64 0) -> (w64 1074044016)])
  // Module DataplaneMapTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074043768)[ -> (w64 1074076000)])
  // Module DataplaneDchainTableAllocate
  // BDD node 3:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074043776)[(w64 0) -> (w64 1074076424)])
  // Module DataplaneVectorTableAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074043784)[(w64 0) -> (w64 1074093640)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_borrow(vector:(w64 1074076424), index:(w32 0), val_out:(w64 1074043624)[ -> (w64 1074090320)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074076424), index:(w32 0), value:(w64 1074090320)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_0(4);
  vector_table_1074076424_value_0[0] = 0;
  vector_table_1074076424_value_0[1] = 0;
  vector_table_1074076424_value_0[2] = 0;
  vector_table_1074076424_value_0[3] = 1;
  state->vector_table_1074076424.write(0, vector_table_1074076424_value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074093640), index:(w32 0), val_out:(w64 1074043688)[ -> (w64 1074107536)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074093640), index:(w32 0), value:(w64 1074107536)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_0(2);
  vector_table_1074093640_value_0[0] = 0;
  vector_table_1074093640_value_0[1] = 1;
  state->vector_table_1074093640.write(0, vector_table_1074093640_value_0);
  // BDD node 9:vector_borrow(vector:(w64 1074076424), index:(w32 1), val_out:(w64 1074043624)[ -> (w64 1074090344)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074076424), index:(w32 1), value:(w64 1074090344)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_1(4);
  vector_table_1074076424_value_1[0] = 0;
  vector_table_1074076424_value_1[1] = 0;
  vector_table_1074076424_value_1[2] = 0;
  vector_table_1074076424_value_1[3] = 0;
  state->vector_table_1074076424.write(1, vector_table_1074076424_value_1);
  // BDD node 11:vector_borrow(vector:(w64 1074093640), index:(w32 1), val_out:(w64 1074043688)[ -> (w64 1074107560)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074093640), index:(w32 1), value:(w64 1074107560)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_1(2);
  vector_table_1074093640_value_1[0] = 0;
  vector_table_1074093640_value_1[1] = 0;
  state->vector_table_1074093640.write(1, vector_table_1074093640_value_1);
  // BDD node 13:vector_borrow(vector:(w64 1074076424), index:(w32 2), val_out:(w64 1074043624)[ -> (w64 1074090368)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074076424), index:(w32 2), value:(w64 1074090368)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_2(4);
  vector_table_1074076424_value_2[0] = 0;
  vector_table_1074076424_value_2[1] = 0;
  vector_table_1074076424_value_2[2] = 0;
  vector_table_1074076424_value_2[3] = 1;
  state->vector_table_1074076424.write(2, vector_table_1074076424_value_2);
  // BDD node 15:vector_borrow(vector:(w64 1074093640), index:(w32 2), val_out:(w64 1074043688)[ -> (w64 1074107584)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074093640), index:(w32 2), value:(w64 1074107584)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_2(2);
  vector_table_1074093640_value_2[0] = 0;
  vector_table_1074093640_value_2[1] = 3;
  state->vector_table_1074093640.write(2, vector_table_1074093640_value_2);
  // BDD node 17:vector_borrow(vector:(w64 1074076424), index:(w32 3), val_out:(w64 1074043624)[ -> (w64 1074090392)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074076424), index:(w32 3), value:(w64 1074090392)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_3(4);
  vector_table_1074076424_value_3[0] = 0;
  vector_table_1074076424_value_3[1] = 0;
  vector_table_1074076424_value_3[2] = 0;
  vector_table_1074076424_value_3[3] = 0;
  state->vector_table_1074076424.write(3, vector_table_1074076424_value_3);
  // BDD node 19:vector_borrow(vector:(w64 1074093640), index:(w32 3), val_out:(w64 1074043688)[ -> (w64 1074107608)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074093640), index:(w32 3), value:(w64 1074107608)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_3(2);
  vector_table_1074093640_value_3[0] = 0;
  vector_table_1074093640_value_3[1] = 2;
  state->vector_table_1074093640.write(3, vector_table_1074093640_value_3);
  // BDD node 21:vector_borrow(vector:(w64 1074076424), index:(w32 4), val_out:(w64 1074043624)[ -> (w64 1074090416)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074076424), index:(w32 4), value:(w64 1074090416)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_4(4);
  vector_table_1074076424_value_4[0] = 0;
  vector_table_1074076424_value_4[1] = 0;
  vector_table_1074076424_value_4[2] = 0;
  vector_table_1074076424_value_4[3] = 1;
  state->vector_table_1074076424.write(4, vector_table_1074076424_value_4);
  // BDD node 23:vector_borrow(vector:(w64 1074093640), index:(w32 4), val_out:(w64 1074043688)[ -> (w64 1074107632)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074093640), index:(w32 4), value:(w64 1074107632)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_4(2);
  vector_table_1074093640_value_4[0] = 0;
  vector_table_1074093640_value_4[1] = 5;
  state->vector_table_1074093640.write(4, vector_table_1074093640_value_4);
  // BDD node 25:vector_borrow(vector:(w64 1074076424), index:(w32 5), val_out:(w64 1074043624)[ -> (w64 1074090440)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074076424), index:(w32 5), value:(w64 1074090440)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_5(4);
  vector_table_1074076424_value_5[0] = 0;
  vector_table_1074076424_value_5[1] = 0;
  vector_table_1074076424_value_5[2] = 0;
  vector_table_1074076424_value_5[3] = 0;
  state->vector_table_1074076424.write(5, vector_table_1074076424_value_5);
  // BDD node 27:vector_borrow(vector:(w64 1074093640), index:(w32 5), val_out:(w64 1074043688)[ -> (w64 1074107656)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074093640), index:(w32 5), value:(w64 1074107656)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_5(2);
  vector_table_1074093640_value_5[0] = 0;
  vector_table_1074093640_value_5[1] = 4;
  state->vector_table_1074093640.write(5, vector_table_1074093640_value_5);
  // BDD node 29:vector_borrow(vector:(w64 1074076424), index:(w32 6), val_out:(w64 1074043624)[ -> (w64 1074090464)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074076424), index:(w32 6), value:(w64 1074090464)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_6(4);
  vector_table_1074076424_value_6[0] = 0;
  vector_table_1074076424_value_6[1] = 0;
  vector_table_1074076424_value_6[2] = 0;
  vector_table_1074076424_value_6[3] = 1;
  state->vector_table_1074076424.write(6, vector_table_1074076424_value_6);
  // BDD node 31:vector_borrow(vector:(w64 1074093640), index:(w32 6), val_out:(w64 1074043688)[ -> (w64 1074107680)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074093640), index:(w32 6), value:(w64 1074107680)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_6(2);
  vector_table_1074093640_value_6[0] = 0;
  vector_table_1074093640_value_6[1] = 7;
  state->vector_table_1074093640.write(6, vector_table_1074093640_value_6);
  // BDD node 33:vector_borrow(vector:(w64 1074076424), index:(w32 7), val_out:(w64 1074043624)[ -> (w64 1074090488)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074076424), index:(w32 7), value:(w64 1074090488)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_7(4);
  vector_table_1074076424_value_7[0] = 0;
  vector_table_1074076424_value_7[1] = 0;
  vector_table_1074076424_value_7[2] = 0;
  vector_table_1074076424_value_7[3] = 0;
  state->vector_table_1074076424.write(7, vector_table_1074076424_value_7);
  // BDD node 35:vector_borrow(vector:(w64 1074093640), index:(w32 7), val_out:(w64 1074043688)[ -> (w64 1074107704)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074093640), index:(w32 7), value:(w64 1074107704)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_7(2);
  vector_table_1074093640_value_7[0] = 0;
  vector_table_1074093640_value_7[1] = 6;
  state->vector_table_1074093640.write(7, vector_table_1074093640_value_7);
  // BDD node 37:vector_borrow(vector:(w64 1074076424), index:(w32 8), val_out:(w64 1074043624)[ -> (w64 1074090512)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074076424), index:(w32 8), value:(w64 1074090512)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_8(4);
  vector_table_1074076424_value_8[0] = 0;
  vector_table_1074076424_value_8[1] = 0;
  vector_table_1074076424_value_8[2] = 0;
  vector_table_1074076424_value_8[3] = 1;
  state->vector_table_1074076424.write(8, vector_table_1074076424_value_8);
  // BDD node 39:vector_borrow(vector:(w64 1074093640), index:(w32 8), val_out:(w64 1074043688)[ -> (w64 1074107728)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074093640), index:(w32 8), value:(w64 1074107728)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_8(2);
  vector_table_1074093640_value_8[0] = 0;
  vector_table_1074093640_value_8[1] = 9;
  state->vector_table_1074093640.write(8, vector_table_1074093640_value_8);
  // BDD node 41:vector_borrow(vector:(w64 1074076424), index:(w32 9), val_out:(w64 1074043624)[ -> (w64 1074090536)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074076424), index:(w32 9), value:(w64 1074090536)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_9(4);
  vector_table_1074076424_value_9[0] = 0;
  vector_table_1074076424_value_9[1] = 0;
  vector_table_1074076424_value_9[2] = 0;
  vector_table_1074076424_value_9[3] = 0;
  state->vector_table_1074076424.write(9, vector_table_1074076424_value_9);
  // BDD node 43:vector_borrow(vector:(w64 1074093640), index:(w32 9), val_out:(w64 1074043688)[ -> (w64 1074107752)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074093640), index:(w32 9), value:(w64 1074107752)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_9(2);
  vector_table_1074093640_value_9[0] = 0;
  vector_table_1074093640_value_9[1] = 8;
  state->vector_table_1074093640.write(9, vector_table_1074093640_value_9);
  // BDD node 45:vector_borrow(vector:(w64 1074076424), index:(w32 10), val_out:(w64 1074043624)[ -> (w64 1074090560)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074076424), index:(w32 10), value:(w64 1074090560)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_10(4);
  vector_table_1074076424_value_10[0] = 0;
  vector_table_1074076424_value_10[1] = 0;
  vector_table_1074076424_value_10[2] = 0;
  vector_table_1074076424_value_10[3] = 1;
  state->vector_table_1074076424.write(10, vector_table_1074076424_value_10);
  // BDD node 47:vector_borrow(vector:(w64 1074093640), index:(w32 10), val_out:(w64 1074043688)[ -> (w64 1074107776)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074093640), index:(w32 10), value:(w64 1074107776)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_10(2);
  vector_table_1074093640_value_10[0] = 0;
  vector_table_1074093640_value_10[1] = 11;
  state->vector_table_1074093640.write(10, vector_table_1074093640_value_10);
  // BDD node 49:vector_borrow(vector:(w64 1074076424), index:(w32 11), val_out:(w64 1074043624)[ -> (w64 1074090584)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074076424), index:(w32 11), value:(w64 1074090584)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_11(4);
  vector_table_1074076424_value_11[0] = 0;
  vector_table_1074076424_value_11[1] = 0;
  vector_table_1074076424_value_11[2] = 0;
  vector_table_1074076424_value_11[3] = 0;
  state->vector_table_1074076424.write(11, vector_table_1074076424_value_11);
  // BDD node 51:vector_borrow(vector:(w64 1074093640), index:(w32 11), val_out:(w64 1074043688)[ -> (w64 1074107800)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074093640), index:(w32 11), value:(w64 1074107800)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_11(2);
  vector_table_1074093640_value_11[0] = 0;
  vector_table_1074093640_value_11[1] = 10;
  state->vector_table_1074093640.write(11, vector_table_1074093640_value_11);
  // BDD node 53:vector_borrow(vector:(w64 1074076424), index:(w32 12), val_out:(w64 1074043624)[ -> (w64 1074090608)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074076424), index:(w32 12), value:(w64 1074090608)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_12(4);
  vector_table_1074076424_value_12[0] = 0;
  vector_table_1074076424_value_12[1] = 0;
  vector_table_1074076424_value_12[2] = 0;
  vector_table_1074076424_value_12[3] = 1;
  state->vector_table_1074076424.write(12, vector_table_1074076424_value_12);
  // BDD node 55:vector_borrow(vector:(w64 1074093640), index:(w32 12), val_out:(w64 1074043688)[ -> (w64 1074107824)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074093640), index:(w32 12), value:(w64 1074107824)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_12(2);
  vector_table_1074093640_value_12[0] = 0;
  vector_table_1074093640_value_12[1] = 13;
  state->vector_table_1074093640.write(12, vector_table_1074093640_value_12);
  // BDD node 57:vector_borrow(vector:(w64 1074076424), index:(w32 13), val_out:(w64 1074043624)[ -> (w64 1074090632)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074076424), index:(w32 13), value:(w64 1074090632)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_13(4);
  vector_table_1074076424_value_13[0] = 0;
  vector_table_1074076424_value_13[1] = 0;
  vector_table_1074076424_value_13[2] = 0;
  vector_table_1074076424_value_13[3] = 0;
  state->vector_table_1074076424.write(13, vector_table_1074076424_value_13);
  // BDD node 59:vector_borrow(vector:(w64 1074093640), index:(w32 13), val_out:(w64 1074043688)[ -> (w64 1074107848)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074093640), index:(w32 13), value:(w64 1074107848)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_13(2);
  vector_table_1074093640_value_13[0] = 0;
  vector_table_1074093640_value_13[1] = 12;
  state->vector_table_1074093640.write(13, vector_table_1074093640_value_13);
  // BDD node 61:vector_borrow(vector:(w64 1074076424), index:(w32 14), val_out:(w64 1074043624)[ -> (w64 1074090656)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074076424), index:(w32 14), value:(w64 1074090656)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_14(4);
  vector_table_1074076424_value_14[0] = 0;
  vector_table_1074076424_value_14[1] = 0;
  vector_table_1074076424_value_14[2] = 0;
  vector_table_1074076424_value_14[3] = 1;
  state->vector_table_1074076424.write(14, vector_table_1074076424_value_14);
  // BDD node 63:vector_borrow(vector:(w64 1074093640), index:(w32 14), val_out:(w64 1074043688)[ -> (w64 1074107872)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074093640), index:(w32 14), value:(w64 1074107872)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_14(2);
  vector_table_1074093640_value_14[0] = 0;
  vector_table_1074093640_value_14[1] = 15;
  state->vector_table_1074093640.write(14, vector_table_1074093640_value_14);
  // BDD node 65:vector_borrow(vector:(w64 1074076424), index:(w32 15), val_out:(w64 1074043624)[ -> (w64 1074090680)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074076424), index:(w32 15), value:(w64 1074090680)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_15(4);
  vector_table_1074076424_value_15[0] = 0;
  vector_table_1074076424_value_15[1] = 0;
  vector_table_1074076424_value_15[2] = 0;
  vector_table_1074076424_value_15[3] = 0;
  state->vector_table_1074076424.write(15, vector_table_1074076424_value_15);
  // BDD node 67:vector_borrow(vector:(w64 1074093640), index:(w32 15), val_out:(w64 1074043688)[ -> (w64 1074107896)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074093640), index:(w32 15), value:(w64 1074107896)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_15(2);
  vector_table_1074093640_value_15[0] = 0;
  vector_table_1074093640_value_15[1] = 14;
  state->vector_table_1074093640.write(15, vector_table_1074093640_value_15);
  // BDD node 69:vector_borrow(vector:(w64 1074076424), index:(w32 16), val_out:(w64 1074043624)[ -> (w64 1074090704)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074076424), index:(w32 16), value:(w64 1074090704)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_16(4);
  vector_table_1074076424_value_16[0] = 0;
  vector_table_1074076424_value_16[1] = 0;
  vector_table_1074076424_value_16[2] = 0;
  vector_table_1074076424_value_16[3] = 1;
  state->vector_table_1074076424.write(16, vector_table_1074076424_value_16);
  // BDD node 71:vector_borrow(vector:(w64 1074093640), index:(w32 16), val_out:(w64 1074043688)[ -> (w64 1074107920)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074093640), index:(w32 16), value:(w64 1074107920)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_16(2);
  vector_table_1074093640_value_16[0] = 0;
  vector_table_1074093640_value_16[1] = 17;
  state->vector_table_1074093640.write(16, vector_table_1074093640_value_16);
  // BDD node 73:vector_borrow(vector:(w64 1074076424), index:(w32 17), val_out:(w64 1074043624)[ -> (w64 1074090728)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074076424), index:(w32 17), value:(w64 1074090728)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_17(4);
  vector_table_1074076424_value_17[0] = 0;
  vector_table_1074076424_value_17[1] = 0;
  vector_table_1074076424_value_17[2] = 0;
  vector_table_1074076424_value_17[3] = 0;
  state->vector_table_1074076424.write(17, vector_table_1074076424_value_17);
  // BDD node 75:vector_borrow(vector:(w64 1074093640), index:(w32 17), val_out:(w64 1074043688)[ -> (w64 1074107944)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074093640), index:(w32 17), value:(w64 1074107944)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_17(2);
  vector_table_1074093640_value_17[0] = 0;
  vector_table_1074093640_value_17[1] = 16;
  state->vector_table_1074093640.write(17, vector_table_1074093640_value_17);
  // BDD node 77:vector_borrow(vector:(w64 1074076424), index:(w32 18), val_out:(w64 1074043624)[ -> (w64 1074090752)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074076424), index:(w32 18), value:(w64 1074090752)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_18(4);
  vector_table_1074076424_value_18[0] = 0;
  vector_table_1074076424_value_18[1] = 0;
  vector_table_1074076424_value_18[2] = 0;
  vector_table_1074076424_value_18[3] = 1;
  state->vector_table_1074076424.write(18, vector_table_1074076424_value_18);
  // BDD node 79:vector_borrow(vector:(w64 1074093640), index:(w32 18), val_out:(w64 1074043688)[ -> (w64 1074107968)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074093640), index:(w32 18), value:(w64 1074107968)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_18(2);
  vector_table_1074093640_value_18[0] = 0;
  vector_table_1074093640_value_18[1] = 19;
  state->vector_table_1074093640.write(18, vector_table_1074093640_value_18);
  // BDD node 81:vector_borrow(vector:(w64 1074076424), index:(w32 19), val_out:(w64 1074043624)[ -> (w64 1074090776)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074076424), index:(w32 19), value:(w64 1074090776)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_19(4);
  vector_table_1074076424_value_19[0] = 0;
  vector_table_1074076424_value_19[1] = 0;
  vector_table_1074076424_value_19[2] = 0;
  vector_table_1074076424_value_19[3] = 0;
  state->vector_table_1074076424.write(19, vector_table_1074076424_value_19);
  // BDD node 83:vector_borrow(vector:(w64 1074093640), index:(w32 19), val_out:(w64 1074043688)[ -> (w64 1074107992)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074093640), index:(w32 19), value:(w64 1074107992)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_19(2);
  vector_table_1074093640_value_19[0] = 0;
  vector_table_1074093640_value_19[1] = 18;
  state->vector_table_1074093640.write(19, vector_table_1074093640_value_19);
  // BDD node 85:vector_borrow(vector:(w64 1074076424), index:(w32 20), val_out:(w64 1074043624)[ -> (w64 1074090800)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074076424), index:(w32 20), value:(w64 1074090800)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_20(4);
  vector_table_1074076424_value_20[0] = 0;
  vector_table_1074076424_value_20[1] = 0;
  vector_table_1074076424_value_20[2] = 0;
  vector_table_1074076424_value_20[3] = 1;
  state->vector_table_1074076424.write(20, vector_table_1074076424_value_20);
  // BDD node 87:vector_borrow(vector:(w64 1074093640), index:(w32 20), val_out:(w64 1074043688)[ -> (w64 1074108016)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074093640), index:(w32 20), value:(w64 1074108016)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_20(2);
  vector_table_1074093640_value_20[0] = 0;
  vector_table_1074093640_value_20[1] = 21;
  state->vector_table_1074093640.write(20, vector_table_1074093640_value_20);
  // BDD node 89:vector_borrow(vector:(w64 1074076424), index:(w32 21), val_out:(w64 1074043624)[ -> (w64 1074090824)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074076424), index:(w32 21), value:(w64 1074090824)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_21(4);
  vector_table_1074076424_value_21[0] = 0;
  vector_table_1074076424_value_21[1] = 0;
  vector_table_1074076424_value_21[2] = 0;
  vector_table_1074076424_value_21[3] = 0;
  state->vector_table_1074076424.write(21, vector_table_1074076424_value_21);
  // BDD node 91:vector_borrow(vector:(w64 1074093640), index:(w32 21), val_out:(w64 1074043688)[ -> (w64 1074108040)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074093640), index:(w32 21), value:(w64 1074108040)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_21(2);
  vector_table_1074093640_value_21[0] = 0;
  vector_table_1074093640_value_21[1] = 20;
  state->vector_table_1074093640.write(21, vector_table_1074093640_value_21);
  // BDD node 93:vector_borrow(vector:(w64 1074076424), index:(w32 22), val_out:(w64 1074043624)[ -> (w64 1074090848)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074076424), index:(w32 22), value:(w64 1074090848)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_22(4);
  vector_table_1074076424_value_22[0] = 0;
  vector_table_1074076424_value_22[1] = 0;
  vector_table_1074076424_value_22[2] = 0;
  vector_table_1074076424_value_22[3] = 1;
  state->vector_table_1074076424.write(22, vector_table_1074076424_value_22);
  // BDD node 95:vector_borrow(vector:(w64 1074093640), index:(w32 22), val_out:(w64 1074043688)[ -> (w64 1074108064)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074093640), index:(w32 22), value:(w64 1074108064)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_22(2);
  vector_table_1074093640_value_22[0] = 0;
  vector_table_1074093640_value_22[1] = 23;
  state->vector_table_1074093640.write(22, vector_table_1074093640_value_22);
  // BDD node 97:vector_borrow(vector:(w64 1074076424), index:(w32 23), val_out:(w64 1074043624)[ -> (w64 1074090872)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074076424), index:(w32 23), value:(w64 1074090872)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_23(4);
  vector_table_1074076424_value_23[0] = 0;
  vector_table_1074076424_value_23[1] = 0;
  vector_table_1074076424_value_23[2] = 0;
  vector_table_1074076424_value_23[3] = 0;
  state->vector_table_1074076424.write(23, vector_table_1074076424_value_23);
  // BDD node 99:vector_borrow(vector:(w64 1074093640), index:(w32 23), val_out:(w64 1074043688)[ -> (w64 1074108088)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074093640), index:(w32 23), value:(w64 1074108088)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_23(2);
  vector_table_1074093640_value_23[0] = 0;
  vector_table_1074093640_value_23[1] = 22;
  state->vector_table_1074093640.write(23, vector_table_1074093640_value_23);
  // BDD node 101:vector_borrow(vector:(w64 1074076424), index:(w32 24), val_out:(w64 1074043624)[ -> (w64 1074090896)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074076424), index:(w32 24), value:(w64 1074090896)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_24(4);
  vector_table_1074076424_value_24[0] = 0;
  vector_table_1074076424_value_24[1] = 0;
  vector_table_1074076424_value_24[2] = 0;
  vector_table_1074076424_value_24[3] = 1;
  state->vector_table_1074076424.write(24, vector_table_1074076424_value_24);
  // BDD node 103:vector_borrow(vector:(w64 1074093640), index:(w32 24), val_out:(w64 1074043688)[ -> (w64 1074108112)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074093640), index:(w32 24), value:(w64 1074108112)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_24(2);
  vector_table_1074093640_value_24[0] = 0;
  vector_table_1074093640_value_24[1] = 25;
  state->vector_table_1074093640.write(24, vector_table_1074093640_value_24);
  // BDD node 105:vector_borrow(vector:(w64 1074076424), index:(w32 25), val_out:(w64 1074043624)[ -> (w64 1074090920)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074076424), index:(w32 25), value:(w64 1074090920)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_25(4);
  vector_table_1074076424_value_25[0] = 0;
  vector_table_1074076424_value_25[1] = 0;
  vector_table_1074076424_value_25[2] = 0;
  vector_table_1074076424_value_25[3] = 0;
  state->vector_table_1074076424.write(25, vector_table_1074076424_value_25);
  // BDD node 107:vector_borrow(vector:(w64 1074093640), index:(w32 25), val_out:(w64 1074043688)[ -> (w64 1074108136)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074093640), index:(w32 25), value:(w64 1074108136)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_25(2);
  vector_table_1074093640_value_25[0] = 0;
  vector_table_1074093640_value_25[1] = 24;
  state->vector_table_1074093640.write(25, vector_table_1074093640_value_25);
  // BDD node 109:vector_borrow(vector:(w64 1074076424), index:(w32 26), val_out:(w64 1074043624)[ -> (w64 1074090944)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074076424), index:(w32 26), value:(w64 1074090944)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_26(4);
  vector_table_1074076424_value_26[0] = 0;
  vector_table_1074076424_value_26[1] = 0;
  vector_table_1074076424_value_26[2] = 0;
  vector_table_1074076424_value_26[3] = 1;
  state->vector_table_1074076424.write(26, vector_table_1074076424_value_26);
  // BDD node 111:vector_borrow(vector:(w64 1074093640), index:(w32 26), val_out:(w64 1074043688)[ -> (w64 1074108160)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074093640), index:(w32 26), value:(w64 1074108160)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_26(2);
  vector_table_1074093640_value_26[0] = 0;
  vector_table_1074093640_value_26[1] = 27;
  state->vector_table_1074093640.write(26, vector_table_1074093640_value_26);
  // BDD node 113:vector_borrow(vector:(w64 1074076424), index:(w32 27), val_out:(w64 1074043624)[ -> (w64 1074090968)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074076424), index:(w32 27), value:(w64 1074090968)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_27(4);
  vector_table_1074076424_value_27[0] = 0;
  vector_table_1074076424_value_27[1] = 0;
  vector_table_1074076424_value_27[2] = 0;
  vector_table_1074076424_value_27[3] = 0;
  state->vector_table_1074076424.write(27, vector_table_1074076424_value_27);
  // BDD node 115:vector_borrow(vector:(w64 1074093640), index:(w32 27), val_out:(w64 1074043688)[ -> (w64 1074108184)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074093640), index:(w32 27), value:(w64 1074108184)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_27(2);
  vector_table_1074093640_value_27[0] = 0;
  vector_table_1074093640_value_27[1] = 26;
  state->vector_table_1074093640.write(27, vector_table_1074093640_value_27);
  // BDD node 117:vector_borrow(vector:(w64 1074076424), index:(w32 28), val_out:(w64 1074043624)[ -> (w64 1074090992)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074076424), index:(w32 28), value:(w64 1074090992)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_28(4);
  vector_table_1074076424_value_28[0] = 0;
  vector_table_1074076424_value_28[1] = 0;
  vector_table_1074076424_value_28[2] = 0;
  vector_table_1074076424_value_28[3] = 1;
  state->vector_table_1074076424.write(28, vector_table_1074076424_value_28);
  // BDD node 119:vector_borrow(vector:(w64 1074093640), index:(w32 28), val_out:(w64 1074043688)[ -> (w64 1074108208)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074093640), index:(w32 28), value:(w64 1074108208)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_28(2);
  vector_table_1074093640_value_28[0] = 0;
  vector_table_1074093640_value_28[1] = 29;
  state->vector_table_1074093640.write(28, vector_table_1074093640_value_28);
  // BDD node 121:vector_borrow(vector:(w64 1074076424), index:(w32 29), val_out:(w64 1074043624)[ -> (w64 1074091016)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074076424), index:(w32 29), value:(w64 1074091016)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_29(4);
  vector_table_1074076424_value_29[0] = 0;
  vector_table_1074076424_value_29[1] = 0;
  vector_table_1074076424_value_29[2] = 0;
  vector_table_1074076424_value_29[3] = 0;
  state->vector_table_1074076424.write(29, vector_table_1074076424_value_29);
  // BDD node 123:vector_borrow(vector:(w64 1074093640), index:(w32 29), val_out:(w64 1074043688)[ -> (w64 1074108232)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074093640), index:(w32 29), value:(w64 1074108232)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_29(2);
  vector_table_1074093640_value_29[0] = 0;
  vector_table_1074093640_value_29[1] = 28;
  state->vector_table_1074093640.write(29, vector_table_1074093640_value_29);
  // BDD node 125:vector_borrow(vector:(w64 1074076424), index:(w32 30), val_out:(w64 1074043624)[ -> (w64 1074091040)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074076424), index:(w32 30), value:(w64 1074091040)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_30(4);
  vector_table_1074076424_value_30[0] = 0;
  vector_table_1074076424_value_30[1] = 0;
  vector_table_1074076424_value_30[2] = 0;
  vector_table_1074076424_value_30[3] = 1;
  state->vector_table_1074076424.write(30, vector_table_1074076424_value_30);
  // BDD node 127:vector_borrow(vector:(w64 1074093640), index:(w32 30), val_out:(w64 1074043688)[ -> (w64 1074108256)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074093640), index:(w32 30), value:(w64 1074108256)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_30(2);
  vector_table_1074093640_value_30[0] = 0;
  vector_table_1074093640_value_30[1] = 31;
  state->vector_table_1074093640.write(30, vector_table_1074093640_value_30);
  // BDD node 129:vector_borrow(vector:(w64 1074076424), index:(w32 31), val_out:(w64 1074043624)[ -> (w64 1074091064)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074076424), index:(w32 31), value:(w64 1074091064)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074076424_value_31(4);
  vector_table_1074076424_value_31[0] = 0;
  vector_table_1074076424_value_31[1] = 0;
  vector_table_1074076424_value_31[2] = 0;
  vector_table_1074076424_value_31[3] = 0;
  state->vector_table_1074076424.write(31, vector_table_1074076424_value_31);
  // BDD node 131:vector_borrow(vector:(w64 1074093640), index:(w32 31), val_out:(w64 1074043688)[ -> (w64 1074108280)])
  // Module Ignore
  // BDD node 132:vector_return(vector:(w64 1074093640), index:(w32 31), value:(w64 1074108280)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074093640_value_31(2);
  vector_table_1074093640_value_31[0] = 0;
  vector_table_1074093640_value_31[1] = 30;
  state->vector_table_1074093640.write(31, vector_table_1074093640_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 vector_data_r11;
  u32 map_has_this_key;
  u32 allocated_index;
  u16 vector_data_r68;
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

  if (bswap16(cpu_hdr->code_path) == 2789) {
    // EP node  2775
    // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  2776
    // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  2777
    // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  2778
    // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t value_0;
    state->vector_table_1074076424.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  2779
    // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    buffer_t value_1;
    state->vector_table_1074093640.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_1);
    // EP node  2780
    // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  2781
      // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  2783
      // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    } else {
      // EP node  2782
      // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  2784
      // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      buffer_t map_table_1074044016_key_0(13);
      map_table_1074044016_key_0[0] = *(u8*)(hdr_2 + 0);
      map_table_1074044016_key_0[1] = *(u8*)(hdr_2 + 1);
      map_table_1074044016_key_0[2] = *(u8*)(hdr_2 + 2);
      map_table_1074044016_key_0[3] = *(u8*)(hdr_2 + 3);
      map_table_1074044016_key_0[4] = *(u8*)(hdr_1 + 12);
      map_table_1074044016_key_0[5] = *(u8*)(hdr_1 + 13);
      map_table_1074044016_key_0[6] = *(u8*)(hdr_1 + 14);
      map_table_1074044016_key_0[7] = *(u8*)(hdr_1 + 15);
      map_table_1074044016_key_0[8] = *(u8*)(hdr_1 + 16);
      map_table_1074044016_key_0[9] = *(u8*)(hdr_1 + 17);
      map_table_1074044016_key_0[10] = *(u8*)(hdr_1 + 18);
      map_table_1074044016_key_0[11] = *(u8*)(hdr_1 + 19);
      map_table_1074044016_key_0[12] = *(u8*)(hdr_1 + 9);
      u32 value_2;
      bool found_0 = state->map_table_1074044016.get(map_table_1074044016_key_0, value_2);
      // EP node  2785
      // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_0)) {
        // EP node  2786
        // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  10466
        // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        u32 allocated_index_1;
        bool success_0 = state->dchain_table_1074076000.allocate_new_index(allocated_index_1);
        // EP node  10524
        // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
        if ((0) == (success_0)) {
          // EP node  10525
          // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
          // EP node  11280
          // BDD node 162:vector_return(vector:(w64 1074093640), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107536)[(ReadLSB w16 (w32 0) vector_data_r68)])
          // EP node  11749
          // BDD node 166:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
          if ((65535) != ((u16)value_1.get(0, 2))) {
            // EP node  11750
            // BDD node 166:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
            // EP node  12249
            // BDD node 167:FORWARD
            cpu_hdr->egress_dev = bswap16((u16)value_1.get(0, 2));
          } else {
            // EP node  11751
            // BDD node 166:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
            // EP node  12103
            // BDD node 168:DROP
            result.forward = false;
          }
        } else {
          // EP node  10526
          // BDD node 160:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
          // EP node  10646
          // BDD node 170:map_put(map:(w64 1074044016), key:(w64 1074071736)[(Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks)))))))))) -> (Concat w104 (Read w8 (w32 265) packet_chunks) (Concat w96 (Read w8 (w32 275) packet_chunks) (Concat w88 (Read w8 (w32 274) packet_chunks) (Concat w80 (Read w8 (w32 273) packet_chunks) (Concat w72 (Read w8 (w32 272) packet_chunks) (Concat w64 (Read w8 (w32 271) packet_chunks) (Concat w56 (Read w8 (w32 270) packet_chunks) (Concat w48 (Read w8 (w32 269) packet_chunks) (Concat w40 (Read w8 (w32 268) packet_chunks) (ReadLSB w32 (w32 512) packet_chunks))))))))))], value:(ReadLSB w32 (w32 0) new_index))
          buffer_t map_table_1074044016_key_1(13);
          map_table_1074044016_key_1[0] = *(u8*)(hdr_2 + 0);
          map_table_1074044016_key_1[1] = *(u8*)(hdr_2 + 1);
          map_table_1074044016_key_1[2] = *(u8*)(hdr_2 + 2);
          map_table_1074044016_key_1[3] = *(u8*)(hdr_2 + 3);
          map_table_1074044016_key_1[4] = *(u8*)(hdr_1 + 12);
          map_table_1074044016_key_1[5] = *(u8*)(hdr_1 + 13);
          map_table_1074044016_key_1[6] = *(u8*)(hdr_1 + 14);
          map_table_1074044016_key_1[7] = *(u8*)(hdr_1 + 15);
          map_table_1074044016_key_1[8] = *(u8*)(hdr_1 + 16);
          map_table_1074044016_key_1[9] = *(u8*)(hdr_1 + 17);
          map_table_1074044016_key_1[10] = *(u8*)(hdr_1 + 18);
          map_table_1074044016_key_1[11] = *(u8*)(hdr_1 + 19);
          map_table_1074044016_key_1[12] = *(u8*)(hdr_1 + 9);
          state->map_table_1074044016.put(map_table_1074044016_key_1, allocated_index_1);
          // EP node  10770
          // BDD node 173:vector_return(vector:(w64 1074093640), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074107536)[(ReadLSB w16 (w32 0) vector_data_r68)])
          // EP node  11148
          // BDD node 177:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
          if ((65535) != ((u16)value_1.get(0, 2))) {
            // EP node  11149
            // BDD node 177:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
            // EP node  12102
            // BDD node 178:FORWARD
            cpu_hdr->egress_dev = bswap16((u16)value_1.get(0, 2));
          } else {
            // EP node  11150
            // BDD node 177:if ((Eq false (Eq (w16 65535) (ReadLSB w16 (w32 0) vector_data_r68)))
            // EP node  11889
            // BDD node 179:DROP
            result.forward = false;
          }
        }
      } else {
        // EP node  2787
        // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  2788
        // BDD node 159:dchain_allocate_new_index(chain:(w64 1074076000), index_out:(w64 1074211520)[(ReadLSB w32 (w32 0) allocated_index) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
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
