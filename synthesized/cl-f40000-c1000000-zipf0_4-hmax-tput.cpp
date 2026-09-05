#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  MapSetTable map_set_table_1074047984;
  CountMinSketch cms_1074080384;
  VectorTable vector_table_1074092960;
  VectorTable vector_table_1074110176;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      map_set_table_1074047984("map_set_table_1074047984",{"Ingress.map_set_table_1074047984_144",}, 1000LL),
      cms_1074080384("cms_1074080384",{"Ingress.cms_1074080384_row_0", "Ingress.cms_1074080384_row_1", "Ingress.cms_1074080384_row_2", "Ingress.cms_1074080384_row_3", }, 10000LL),
      vector_table_1074092960("vector_table_1074092960",{"Ingress.vector_table_1074092960_141",}),
      vector_table_1074110176("vector_table_1074110176",{"Ingress.vector_table_1074110176_177","Ingress.vector_table_1074110176_171",})
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
  // BDD node 0:map_allocate(capacity:(w32 65536), key_size:(w32 12), map_out:(w64 1074047712)[(w64 0) -> (w64 1074047984)])
  // Module DataplaneMapSetTableAllocate
  // BDD node 2:dchain_allocate(index_range:(w32 65536), chain_out:(w64 1074047728)[ -> (w64 1074079968)])
  // Module Ignore
  // BDD node 3:cms_allocate(height:(w32 4), width:(w32 1024), key_size:(w16 8), cms_out:(w64 1074047736)[(w64 0) -> (w64 1074080384)], cleanup_interval:(w64 10000000000))
  // Module DataplaneCMSAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074047744)[(w64 0) -> (w64 1074092960)])
  // Module DataplaneVectorTableAllocate
  // BDD node 5:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074047752)[(w64 0) -> (w64 1074110176)])
  // Module DataplaneVectorTableAllocate
  // BDD node 6:vector_borrow(vector:(w64 1074092960), index:(w32 0), val_out:(w64 1074047584)[ -> (w64 1074106856)])
  // Module Ignore
  // BDD node 7:vector_return(vector:(w64 1074092960), index:(w32 0), value:(w64 1074106856)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_0(4);
  vector_table_1074092960_value_0.set(0, 4, 1);
  state->vector_table_1074092960.write(0, vector_table_1074092960_value_0);
  // BDD node 8:vector_borrow(vector:(w64 1074110176), index:(w32 0), val_out:(w64 1074047648)[ -> (w64 1074124072)])
  // Module Ignore
  // BDD node 9:vector_return(vector:(w64 1074110176), index:(w32 0), value:(w64 1074124072)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_0(2);
  vector_table_1074110176_value_0.set(0, 2, 1);
  state->vector_table_1074110176.write(0, vector_table_1074110176_value_0);
  // BDD node 10:vector_borrow(vector:(w64 1074092960), index:(w32 1), val_out:(w64 1074047584)[ -> (w64 1074106880)])
  // Module Ignore
  // BDD node 11:vector_return(vector:(w64 1074092960), index:(w32 1), value:(w64 1074106880)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_1(4);
  vector_table_1074092960_value_1.set(0, 4, 0);
  state->vector_table_1074092960.write(1, vector_table_1074092960_value_1);
  // BDD node 12:vector_borrow(vector:(w64 1074110176), index:(w32 1), val_out:(w64 1074047648)[ -> (w64 1074124096)])
  // Module Ignore
  // BDD node 13:vector_return(vector:(w64 1074110176), index:(w32 1), value:(w64 1074124096)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_1(2);
  vector_table_1074110176_value_1.set(0, 2, 0);
  state->vector_table_1074110176.write(1, vector_table_1074110176_value_1);
  // BDD node 14:vector_borrow(vector:(w64 1074092960), index:(w32 2), val_out:(w64 1074047584)[ -> (w64 1074106904)])
  // Module Ignore
  // BDD node 15:vector_return(vector:(w64 1074092960), index:(w32 2), value:(w64 1074106904)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_2(4);
  vector_table_1074092960_value_2.set(0, 4, 1);
  state->vector_table_1074092960.write(2, vector_table_1074092960_value_2);
  // BDD node 16:vector_borrow(vector:(w64 1074110176), index:(w32 2), val_out:(w64 1074047648)[ -> (w64 1074124120)])
  // Module Ignore
  // BDD node 17:vector_return(vector:(w64 1074110176), index:(w32 2), value:(w64 1074124120)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_2(2);
  vector_table_1074110176_value_2.set(0, 2, 3);
  state->vector_table_1074110176.write(2, vector_table_1074110176_value_2);
  // BDD node 18:vector_borrow(vector:(w64 1074092960), index:(w32 3), val_out:(w64 1074047584)[ -> (w64 1074106928)])
  // Module Ignore
  // BDD node 19:vector_return(vector:(w64 1074092960), index:(w32 3), value:(w64 1074106928)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_3(4);
  vector_table_1074092960_value_3.set(0, 4, 0);
  state->vector_table_1074092960.write(3, vector_table_1074092960_value_3);
  // BDD node 20:vector_borrow(vector:(w64 1074110176), index:(w32 3), val_out:(w64 1074047648)[ -> (w64 1074124144)])
  // Module Ignore
  // BDD node 21:vector_return(vector:(w64 1074110176), index:(w32 3), value:(w64 1074124144)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_3(2);
  vector_table_1074110176_value_3.set(0, 2, 2);
  state->vector_table_1074110176.write(3, vector_table_1074110176_value_3);
  // BDD node 22:vector_borrow(vector:(w64 1074092960), index:(w32 4), val_out:(w64 1074047584)[ -> (w64 1074106952)])
  // Module Ignore
  // BDD node 23:vector_return(vector:(w64 1074092960), index:(w32 4), value:(w64 1074106952)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_4(4);
  vector_table_1074092960_value_4.set(0, 4, 1);
  state->vector_table_1074092960.write(4, vector_table_1074092960_value_4);
  // BDD node 24:vector_borrow(vector:(w64 1074110176), index:(w32 4), val_out:(w64 1074047648)[ -> (w64 1074124168)])
  // Module Ignore
  // BDD node 25:vector_return(vector:(w64 1074110176), index:(w32 4), value:(w64 1074124168)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_4(2);
  vector_table_1074110176_value_4.set(0, 2, 5);
  state->vector_table_1074110176.write(4, vector_table_1074110176_value_4);
  // BDD node 26:vector_borrow(vector:(w64 1074092960), index:(w32 5), val_out:(w64 1074047584)[ -> (w64 1074106976)])
  // Module Ignore
  // BDD node 27:vector_return(vector:(w64 1074092960), index:(w32 5), value:(w64 1074106976)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_5(4);
  vector_table_1074092960_value_5.set(0, 4, 0);
  state->vector_table_1074092960.write(5, vector_table_1074092960_value_5);
  // BDD node 28:vector_borrow(vector:(w64 1074110176), index:(w32 5), val_out:(w64 1074047648)[ -> (w64 1074124192)])
  // Module Ignore
  // BDD node 29:vector_return(vector:(w64 1074110176), index:(w32 5), value:(w64 1074124192)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_5(2);
  vector_table_1074110176_value_5.set(0, 2, 4);
  state->vector_table_1074110176.write(5, vector_table_1074110176_value_5);
  // BDD node 30:vector_borrow(vector:(w64 1074092960), index:(w32 6), val_out:(w64 1074047584)[ -> (w64 1074107000)])
  // Module Ignore
  // BDD node 31:vector_return(vector:(w64 1074092960), index:(w32 6), value:(w64 1074107000)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_6(4);
  vector_table_1074092960_value_6.set(0, 4, 1);
  state->vector_table_1074092960.write(6, vector_table_1074092960_value_6);
  // BDD node 32:vector_borrow(vector:(w64 1074110176), index:(w32 6), val_out:(w64 1074047648)[ -> (w64 1074124216)])
  // Module Ignore
  // BDD node 33:vector_return(vector:(w64 1074110176), index:(w32 6), value:(w64 1074124216)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_6(2);
  vector_table_1074110176_value_6.set(0, 2, 7);
  state->vector_table_1074110176.write(6, vector_table_1074110176_value_6);
  // BDD node 34:vector_borrow(vector:(w64 1074092960), index:(w32 7), val_out:(w64 1074047584)[ -> (w64 1074107024)])
  // Module Ignore
  // BDD node 35:vector_return(vector:(w64 1074092960), index:(w32 7), value:(w64 1074107024)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_7(4);
  vector_table_1074092960_value_7.set(0, 4, 0);
  state->vector_table_1074092960.write(7, vector_table_1074092960_value_7);
  // BDD node 36:vector_borrow(vector:(w64 1074110176), index:(w32 7), val_out:(w64 1074047648)[ -> (w64 1074124240)])
  // Module Ignore
  // BDD node 37:vector_return(vector:(w64 1074110176), index:(w32 7), value:(w64 1074124240)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_7(2);
  vector_table_1074110176_value_7.set(0, 2, 6);
  state->vector_table_1074110176.write(7, vector_table_1074110176_value_7);
  // BDD node 38:vector_borrow(vector:(w64 1074092960), index:(w32 8), val_out:(w64 1074047584)[ -> (w64 1074107048)])
  // Module Ignore
  // BDD node 39:vector_return(vector:(w64 1074092960), index:(w32 8), value:(w64 1074107048)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_8(4);
  vector_table_1074092960_value_8.set(0, 4, 1);
  state->vector_table_1074092960.write(8, vector_table_1074092960_value_8);
  // BDD node 40:vector_borrow(vector:(w64 1074110176), index:(w32 8), val_out:(w64 1074047648)[ -> (w64 1074124264)])
  // Module Ignore
  // BDD node 41:vector_return(vector:(w64 1074110176), index:(w32 8), value:(w64 1074124264)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_8(2);
  vector_table_1074110176_value_8.set(0, 2, 9);
  state->vector_table_1074110176.write(8, vector_table_1074110176_value_8);
  // BDD node 42:vector_borrow(vector:(w64 1074092960), index:(w32 9), val_out:(w64 1074047584)[ -> (w64 1074107072)])
  // Module Ignore
  // BDD node 43:vector_return(vector:(w64 1074092960), index:(w32 9), value:(w64 1074107072)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_9(4);
  vector_table_1074092960_value_9.set(0, 4, 0);
  state->vector_table_1074092960.write(9, vector_table_1074092960_value_9);
  // BDD node 44:vector_borrow(vector:(w64 1074110176), index:(w32 9), val_out:(w64 1074047648)[ -> (w64 1074124288)])
  // Module Ignore
  // BDD node 45:vector_return(vector:(w64 1074110176), index:(w32 9), value:(w64 1074124288)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_9(2);
  vector_table_1074110176_value_9.set(0, 2, 8);
  state->vector_table_1074110176.write(9, vector_table_1074110176_value_9);
  // BDD node 46:vector_borrow(vector:(w64 1074092960), index:(w32 10), val_out:(w64 1074047584)[ -> (w64 1074107096)])
  // Module Ignore
  // BDD node 47:vector_return(vector:(w64 1074092960), index:(w32 10), value:(w64 1074107096)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_10(4);
  vector_table_1074092960_value_10.set(0, 4, 1);
  state->vector_table_1074092960.write(10, vector_table_1074092960_value_10);
  // BDD node 48:vector_borrow(vector:(w64 1074110176), index:(w32 10), val_out:(w64 1074047648)[ -> (w64 1074124312)])
  // Module Ignore
  // BDD node 49:vector_return(vector:(w64 1074110176), index:(w32 10), value:(w64 1074124312)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_10(2);
  vector_table_1074110176_value_10.set(0, 2, 11);
  state->vector_table_1074110176.write(10, vector_table_1074110176_value_10);
  // BDD node 50:vector_borrow(vector:(w64 1074092960), index:(w32 11), val_out:(w64 1074047584)[ -> (w64 1074107120)])
  // Module Ignore
  // BDD node 51:vector_return(vector:(w64 1074092960), index:(w32 11), value:(w64 1074107120)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_11(4);
  vector_table_1074092960_value_11.set(0, 4, 0);
  state->vector_table_1074092960.write(11, vector_table_1074092960_value_11);
  // BDD node 52:vector_borrow(vector:(w64 1074110176), index:(w32 11), val_out:(w64 1074047648)[ -> (w64 1074124336)])
  // Module Ignore
  // BDD node 53:vector_return(vector:(w64 1074110176), index:(w32 11), value:(w64 1074124336)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_11(2);
  vector_table_1074110176_value_11.set(0, 2, 10);
  state->vector_table_1074110176.write(11, vector_table_1074110176_value_11);
  // BDD node 54:vector_borrow(vector:(w64 1074092960), index:(w32 12), val_out:(w64 1074047584)[ -> (w64 1074107144)])
  // Module Ignore
  // BDD node 55:vector_return(vector:(w64 1074092960), index:(w32 12), value:(w64 1074107144)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_12(4);
  vector_table_1074092960_value_12.set(0, 4, 1);
  state->vector_table_1074092960.write(12, vector_table_1074092960_value_12);
  // BDD node 56:vector_borrow(vector:(w64 1074110176), index:(w32 12), val_out:(w64 1074047648)[ -> (w64 1074124360)])
  // Module Ignore
  // BDD node 57:vector_return(vector:(w64 1074110176), index:(w32 12), value:(w64 1074124360)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_12(2);
  vector_table_1074110176_value_12.set(0, 2, 13);
  state->vector_table_1074110176.write(12, vector_table_1074110176_value_12);
  // BDD node 58:vector_borrow(vector:(w64 1074092960), index:(w32 13), val_out:(w64 1074047584)[ -> (w64 1074107168)])
  // Module Ignore
  // BDD node 59:vector_return(vector:(w64 1074092960), index:(w32 13), value:(w64 1074107168)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_13(4);
  vector_table_1074092960_value_13.set(0, 4, 0);
  state->vector_table_1074092960.write(13, vector_table_1074092960_value_13);
  // BDD node 60:vector_borrow(vector:(w64 1074110176), index:(w32 13), val_out:(w64 1074047648)[ -> (w64 1074124384)])
  // Module Ignore
  // BDD node 61:vector_return(vector:(w64 1074110176), index:(w32 13), value:(w64 1074124384)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_13(2);
  vector_table_1074110176_value_13.set(0, 2, 12);
  state->vector_table_1074110176.write(13, vector_table_1074110176_value_13);
  // BDD node 62:vector_borrow(vector:(w64 1074092960), index:(w32 14), val_out:(w64 1074047584)[ -> (w64 1074107192)])
  // Module Ignore
  // BDD node 63:vector_return(vector:(w64 1074092960), index:(w32 14), value:(w64 1074107192)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_14(4);
  vector_table_1074092960_value_14.set(0, 4, 1);
  state->vector_table_1074092960.write(14, vector_table_1074092960_value_14);
  // BDD node 64:vector_borrow(vector:(w64 1074110176), index:(w32 14), val_out:(w64 1074047648)[ -> (w64 1074124408)])
  // Module Ignore
  // BDD node 65:vector_return(vector:(w64 1074110176), index:(w32 14), value:(w64 1074124408)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_14(2);
  vector_table_1074110176_value_14.set(0, 2, 15);
  state->vector_table_1074110176.write(14, vector_table_1074110176_value_14);
  // BDD node 66:vector_borrow(vector:(w64 1074092960), index:(w32 15), val_out:(w64 1074047584)[ -> (w64 1074107216)])
  // Module Ignore
  // BDD node 67:vector_return(vector:(w64 1074092960), index:(w32 15), value:(w64 1074107216)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_15(4);
  vector_table_1074092960_value_15.set(0, 4, 0);
  state->vector_table_1074092960.write(15, vector_table_1074092960_value_15);
  // BDD node 68:vector_borrow(vector:(w64 1074110176), index:(w32 15), val_out:(w64 1074047648)[ -> (w64 1074124432)])
  // Module Ignore
  // BDD node 69:vector_return(vector:(w64 1074110176), index:(w32 15), value:(w64 1074124432)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_15(2);
  vector_table_1074110176_value_15.set(0, 2, 14);
  state->vector_table_1074110176.write(15, vector_table_1074110176_value_15);
  // BDD node 70:vector_borrow(vector:(w64 1074092960), index:(w32 16), val_out:(w64 1074047584)[ -> (w64 1074107240)])
  // Module Ignore
  // BDD node 71:vector_return(vector:(w64 1074092960), index:(w32 16), value:(w64 1074107240)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_16(4);
  vector_table_1074092960_value_16.set(0, 4, 1);
  state->vector_table_1074092960.write(16, vector_table_1074092960_value_16);
  // BDD node 72:vector_borrow(vector:(w64 1074110176), index:(w32 16), val_out:(w64 1074047648)[ -> (w64 1074124456)])
  // Module Ignore
  // BDD node 73:vector_return(vector:(w64 1074110176), index:(w32 16), value:(w64 1074124456)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_16(2);
  vector_table_1074110176_value_16.set(0, 2, 17);
  state->vector_table_1074110176.write(16, vector_table_1074110176_value_16);
  // BDD node 74:vector_borrow(vector:(w64 1074092960), index:(w32 17), val_out:(w64 1074047584)[ -> (w64 1074107264)])
  // Module Ignore
  // BDD node 75:vector_return(vector:(w64 1074092960), index:(w32 17), value:(w64 1074107264)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_17(4);
  vector_table_1074092960_value_17.set(0, 4, 0);
  state->vector_table_1074092960.write(17, vector_table_1074092960_value_17);
  // BDD node 76:vector_borrow(vector:(w64 1074110176), index:(w32 17), val_out:(w64 1074047648)[ -> (w64 1074124480)])
  // Module Ignore
  // BDD node 77:vector_return(vector:(w64 1074110176), index:(w32 17), value:(w64 1074124480)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_17(2);
  vector_table_1074110176_value_17.set(0, 2, 16);
  state->vector_table_1074110176.write(17, vector_table_1074110176_value_17);
  // BDD node 78:vector_borrow(vector:(w64 1074092960), index:(w32 18), val_out:(w64 1074047584)[ -> (w64 1074107288)])
  // Module Ignore
  // BDD node 79:vector_return(vector:(w64 1074092960), index:(w32 18), value:(w64 1074107288)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_18(4);
  vector_table_1074092960_value_18.set(0, 4, 1);
  state->vector_table_1074092960.write(18, vector_table_1074092960_value_18);
  // BDD node 80:vector_borrow(vector:(w64 1074110176), index:(w32 18), val_out:(w64 1074047648)[ -> (w64 1074124504)])
  // Module Ignore
  // BDD node 81:vector_return(vector:(w64 1074110176), index:(w32 18), value:(w64 1074124504)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_18(2);
  vector_table_1074110176_value_18.set(0, 2, 19);
  state->vector_table_1074110176.write(18, vector_table_1074110176_value_18);
  // BDD node 82:vector_borrow(vector:(w64 1074092960), index:(w32 19), val_out:(w64 1074047584)[ -> (w64 1074107312)])
  // Module Ignore
  // BDD node 83:vector_return(vector:(w64 1074092960), index:(w32 19), value:(w64 1074107312)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_19(4);
  vector_table_1074092960_value_19.set(0, 4, 0);
  state->vector_table_1074092960.write(19, vector_table_1074092960_value_19);
  // BDD node 84:vector_borrow(vector:(w64 1074110176), index:(w32 19), val_out:(w64 1074047648)[ -> (w64 1074124528)])
  // Module Ignore
  // BDD node 85:vector_return(vector:(w64 1074110176), index:(w32 19), value:(w64 1074124528)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_19(2);
  vector_table_1074110176_value_19.set(0, 2, 18);
  state->vector_table_1074110176.write(19, vector_table_1074110176_value_19);
  // BDD node 86:vector_borrow(vector:(w64 1074092960), index:(w32 20), val_out:(w64 1074047584)[ -> (w64 1074107336)])
  // Module Ignore
  // BDD node 87:vector_return(vector:(w64 1074092960), index:(w32 20), value:(w64 1074107336)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_20(4);
  vector_table_1074092960_value_20.set(0, 4, 1);
  state->vector_table_1074092960.write(20, vector_table_1074092960_value_20);
  // BDD node 88:vector_borrow(vector:(w64 1074110176), index:(w32 20), val_out:(w64 1074047648)[ -> (w64 1074124552)])
  // Module Ignore
  // BDD node 89:vector_return(vector:(w64 1074110176), index:(w32 20), value:(w64 1074124552)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_20(2);
  vector_table_1074110176_value_20.set(0, 2, 21);
  state->vector_table_1074110176.write(20, vector_table_1074110176_value_20);
  // BDD node 90:vector_borrow(vector:(w64 1074092960), index:(w32 21), val_out:(w64 1074047584)[ -> (w64 1074107360)])
  // Module Ignore
  // BDD node 91:vector_return(vector:(w64 1074092960), index:(w32 21), value:(w64 1074107360)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_21(4);
  vector_table_1074092960_value_21.set(0, 4, 0);
  state->vector_table_1074092960.write(21, vector_table_1074092960_value_21);
  // BDD node 92:vector_borrow(vector:(w64 1074110176), index:(w32 21), val_out:(w64 1074047648)[ -> (w64 1074124576)])
  // Module Ignore
  // BDD node 93:vector_return(vector:(w64 1074110176), index:(w32 21), value:(w64 1074124576)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_21(2);
  vector_table_1074110176_value_21.set(0, 2, 20);
  state->vector_table_1074110176.write(21, vector_table_1074110176_value_21);
  // BDD node 94:vector_borrow(vector:(w64 1074092960), index:(w32 22), val_out:(w64 1074047584)[ -> (w64 1074107384)])
  // Module Ignore
  // BDD node 95:vector_return(vector:(w64 1074092960), index:(w32 22), value:(w64 1074107384)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_22(4);
  vector_table_1074092960_value_22.set(0, 4, 1);
  state->vector_table_1074092960.write(22, vector_table_1074092960_value_22);
  // BDD node 96:vector_borrow(vector:(w64 1074110176), index:(w32 22), val_out:(w64 1074047648)[ -> (w64 1074124600)])
  // Module Ignore
  // BDD node 97:vector_return(vector:(w64 1074110176), index:(w32 22), value:(w64 1074124600)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_22(2);
  vector_table_1074110176_value_22.set(0, 2, 23);
  state->vector_table_1074110176.write(22, vector_table_1074110176_value_22);
  // BDD node 98:vector_borrow(vector:(w64 1074092960), index:(w32 23), val_out:(w64 1074047584)[ -> (w64 1074107408)])
  // Module Ignore
  // BDD node 99:vector_return(vector:(w64 1074092960), index:(w32 23), value:(w64 1074107408)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_23(4);
  vector_table_1074092960_value_23.set(0, 4, 0);
  state->vector_table_1074092960.write(23, vector_table_1074092960_value_23);
  // BDD node 100:vector_borrow(vector:(w64 1074110176), index:(w32 23), val_out:(w64 1074047648)[ -> (w64 1074124624)])
  // Module Ignore
  // BDD node 101:vector_return(vector:(w64 1074110176), index:(w32 23), value:(w64 1074124624)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_23(2);
  vector_table_1074110176_value_23.set(0, 2, 22);
  state->vector_table_1074110176.write(23, vector_table_1074110176_value_23);
  // BDD node 102:vector_borrow(vector:(w64 1074092960), index:(w32 24), val_out:(w64 1074047584)[ -> (w64 1074107432)])
  // Module Ignore
  // BDD node 103:vector_return(vector:(w64 1074092960), index:(w32 24), value:(w64 1074107432)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_24(4);
  vector_table_1074092960_value_24.set(0, 4, 1);
  state->vector_table_1074092960.write(24, vector_table_1074092960_value_24);
  // BDD node 104:vector_borrow(vector:(w64 1074110176), index:(w32 24), val_out:(w64 1074047648)[ -> (w64 1074124648)])
  // Module Ignore
  // BDD node 105:vector_return(vector:(w64 1074110176), index:(w32 24), value:(w64 1074124648)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_24(2);
  vector_table_1074110176_value_24.set(0, 2, 25);
  state->vector_table_1074110176.write(24, vector_table_1074110176_value_24);
  // BDD node 106:vector_borrow(vector:(w64 1074092960), index:(w32 25), val_out:(w64 1074047584)[ -> (w64 1074107456)])
  // Module Ignore
  // BDD node 107:vector_return(vector:(w64 1074092960), index:(w32 25), value:(w64 1074107456)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_25(4);
  vector_table_1074092960_value_25.set(0, 4, 0);
  state->vector_table_1074092960.write(25, vector_table_1074092960_value_25);
  // BDD node 108:vector_borrow(vector:(w64 1074110176), index:(w32 25), val_out:(w64 1074047648)[ -> (w64 1074124672)])
  // Module Ignore
  // BDD node 109:vector_return(vector:(w64 1074110176), index:(w32 25), value:(w64 1074124672)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_25(2);
  vector_table_1074110176_value_25.set(0, 2, 24);
  state->vector_table_1074110176.write(25, vector_table_1074110176_value_25);
  // BDD node 110:vector_borrow(vector:(w64 1074092960), index:(w32 26), val_out:(w64 1074047584)[ -> (w64 1074107480)])
  // Module Ignore
  // BDD node 111:vector_return(vector:(w64 1074092960), index:(w32 26), value:(w64 1074107480)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_26(4);
  vector_table_1074092960_value_26.set(0, 4, 1);
  state->vector_table_1074092960.write(26, vector_table_1074092960_value_26);
  // BDD node 112:vector_borrow(vector:(w64 1074110176), index:(w32 26), val_out:(w64 1074047648)[ -> (w64 1074124696)])
  // Module Ignore
  // BDD node 113:vector_return(vector:(w64 1074110176), index:(w32 26), value:(w64 1074124696)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_26(2);
  vector_table_1074110176_value_26.set(0, 2, 27);
  state->vector_table_1074110176.write(26, vector_table_1074110176_value_26);
  // BDD node 114:vector_borrow(vector:(w64 1074092960), index:(w32 27), val_out:(w64 1074047584)[ -> (w64 1074107504)])
  // Module Ignore
  // BDD node 115:vector_return(vector:(w64 1074092960), index:(w32 27), value:(w64 1074107504)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_27(4);
  vector_table_1074092960_value_27.set(0, 4, 0);
  state->vector_table_1074092960.write(27, vector_table_1074092960_value_27);
  // BDD node 116:vector_borrow(vector:(w64 1074110176), index:(w32 27), val_out:(w64 1074047648)[ -> (w64 1074124720)])
  // Module Ignore
  // BDD node 117:vector_return(vector:(w64 1074110176), index:(w32 27), value:(w64 1074124720)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_27(2);
  vector_table_1074110176_value_27.set(0, 2, 26);
  state->vector_table_1074110176.write(27, vector_table_1074110176_value_27);
  // BDD node 118:vector_borrow(vector:(w64 1074092960), index:(w32 28), val_out:(w64 1074047584)[ -> (w64 1074107528)])
  // Module Ignore
  // BDD node 119:vector_return(vector:(w64 1074092960), index:(w32 28), value:(w64 1074107528)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_28(4);
  vector_table_1074092960_value_28.set(0, 4, 1);
  state->vector_table_1074092960.write(28, vector_table_1074092960_value_28);
  // BDD node 120:vector_borrow(vector:(w64 1074110176), index:(w32 28), val_out:(w64 1074047648)[ -> (w64 1074124744)])
  // Module Ignore
  // BDD node 121:vector_return(vector:(w64 1074110176), index:(w32 28), value:(w64 1074124744)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_28(2);
  vector_table_1074110176_value_28.set(0, 2, 29);
  state->vector_table_1074110176.write(28, vector_table_1074110176_value_28);
  // BDD node 122:vector_borrow(vector:(w64 1074092960), index:(w32 29), val_out:(w64 1074047584)[ -> (w64 1074107552)])
  // Module Ignore
  // BDD node 123:vector_return(vector:(w64 1074092960), index:(w32 29), value:(w64 1074107552)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_29(4);
  vector_table_1074092960_value_29.set(0, 4, 0);
  state->vector_table_1074092960.write(29, vector_table_1074092960_value_29);
  // BDD node 124:vector_borrow(vector:(w64 1074110176), index:(w32 29), val_out:(w64 1074047648)[ -> (w64 1074124768)])
  // Module Ignore
  // BDD node 125:vector_return(vector:(w64 1074110176), index:(w32 29), value:(w64 1074124768)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_29(2);
  vector_table_1074110176_value_29.set(0, 2, 28);
  state->vector_table_1074110176.write(29, vector_table_1074110176_value_29);
  // BDD node 126:vector_borrow(vector:(w64 1074092960), index:(w32 30), val_out:(w64 1074047584)[ -> (w64 1074107576)])
  // Module Ignore
  // BDD node 127:vector_return(vector:(w64 1074092960), index:(w32 30), value:(w64 1074107576)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_30(4);
  vector_table_1074092960_value_30.set(0, 4, 1);
  state->vector_table_1074092960.write(30, vector_table_1074092960_value_30);
  // BDD node 128:vector_borrow(vector:(w64 1074110176), index:(w32 30), val_out:(w64 1074047648)[ -> (w64 1074124792)])
  // Module Ignore
  // BDD node 129:vector_return(vector:(w64 1074110176), index:(w32 30), value:(w64 1074124792)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_30(2);
  vector_table_1074110176_value_30.set(0, 2, 31);
  state->vector_table_1074110176.write(30, vector_table_1074110176_value_30);
  // BDD node 130:vector_borrow(vector:(w64 1074092960), index:(w32 31), val_out:(w64 1074047584)[ -> (w64 1074107600)])
  // Module Ignore
  // BDD node 131:vector_return(vector:(w64 1074092960), index:(w32 31), value:(w64 1074107600)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074092960_value_31(4);
  vector_table_1074092960_value_31.set(0, 4, 0);
  state->vector_table_1074092960.write(31, vector_table_1074092960_value_31);
  // BDD node 132:vector_borrow(vector:(w64 1074110176), index:(w32 31), val_out:(w64 1074047648)[ -> (w64 1074124816)])
  // Module Ignore
  // BDD node 133:vector_return(vector:(w64 1074110176), index:(w32 31), value:(w64 1074124816)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074110176_value_31(2);
  vector_table_1074110176_value_31.set(0, 2, 30);
  state->vector_table_1074110176.write(31, vector_table_1074110176_value_31);

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

  if (bswap16(cpu_hdr->code_path) == 2179) {
    // EP node  2166
    // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  2167
    // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  2168
    // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  2169
    // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    buffer_t value_0;
    state->vector_table_1074092960.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_0);
    // EP node  2170
    // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    buffer_t map_set_table_1074047984_key_0(12);
    map_set_table_1074047984_key_0[0] = *(u8*)(hdr_1 + 12);
    map_set_table_1074047984_key_0[1] = *(u8*)(hdr_1 + 13);
    map_set_table_1074047984_key_0[2] = *(u8*)(hdr_1 + 14);
    map_set_table_1074047984_key_0[3] = *(u8*)(hdr_1 + 15);
    map_set_table_1074047984_key_0[4] = *(u8*)(hdr_1 + 16);
    map_set_table_1074047984_key_0[5] = *(u8*)(hdr_1 + 17);
    map_set_table_1074047984_key_0[6] = *(u8*)(hdr_1 + 18);
    map_set_table_1074047984_key_0[7] = *(u8*)(hdr_1 + 19);
    map_set_table_1074047984_key_0[8] = *(u8*)(hdr_2 + 0);
    map_set_table_1074047984_key_0[9] = *(u8*)(hdr_2 + 1);
    map_set_table_1074047984_key_0[10] = *(u8*)(hdr_2 + 2);
    map_set_table_1074047984_key_0[11] = *(u8*)(hdr_2 + 3);
    bool found_0 = state->map_set_table_1074047984.get(map_set_table_1074047984_key_0);
    // EP node  2171
    // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  2172
      // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
      // EP node  2175
      // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
      if ((0) == (found_0)) {
        // EP node  2176
        // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
        // EP node  2759
        // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
        buffer_t cms_1074080384_key_0(8);
        cms_1074080384_key_0[0] = *(u8*)(hdr_1 + 12);
        cms_1074080384_key_0[1] = *(u8*)(hdr_1 + 13);
        cms_1074080384_key_0[2] = *(u8*)(hdr_1 + 14);
        cms_1074080384_key_0[3] = *(u8*)(hdr_1 + 15);
        cms_1074080384_key_0[4] = *(u8*)(hdr_1 + 16);
        cms_1074080384_key_0[5] = *(u8*)(hdr_1 + 17);
        cms_1074080384_key_0[6] = *(u8*)(hdr_1 + 18);
        cms_1074080384_key_0[7] = *(u8*)(hdr_1 + 19);
        u32 min_estimate_0 = state->cms_1074080384.count_min(cms_1074080384_key_0);
        // EP node  2805
        // BDD node 148:if ((Ule (ReadLSB w32 (w32 0) min_estimate__147) (w32 131071))
        if ((min_estimate_0) <= (131071)) {
          // EP node  2806
          // BDD node 148:if ((Ule (ReadLSB w32 (w32 0) min_estimate__147) (w32 131071))
          // EP node  2854
          // BDD node 149:dchain_allocate_new_index(chain:(w64 1074079968), index_out:(w64 1074225880)[(w32 4294967295) -> (ReadLSB w32 (w32 0) new_index__149)], time:(ReadLSB w64 (w32 0) next_time))
          buffer_t map_set_table_1074047984_key_1(12);
          map_set_table_1074047984_key_1[0] = *(u8*)(hdr_1 + 12);
          map_set_table_1074047984_key_1[1] = *(u8*)(hdr_1 + 13);
          map_set_table_1074047984_key_1[2] = *(u8*)(hdr_1 + 14);
          map_set_table_1074047984_key_1[3] = *(u8*)(hdr_1 + 15);
          map_set_table_1074047984_key_1[4] = *(u8*)(hdr_1 + 16);
          map_set_table_1074047984_key_1[5] = *(u8*)(hdr_1 + 17);
          map_set_table_1074047984_key_1[6] = *(u8*)(hdr_1 + 18);
          map_set_table_1074047984_key_1[7] = *(u8*)(hdr_1 + 19);
          map_set_table_1074047984_key_1[8] = *(u8*)(hdr_2 + 0);
          map_set_table_1074047984_key_1[9] = *(u8*)(hdr_2 + 1);
          map_set_table_1074047984_key_1[10] = *(u8*)(hdr_2 + 2);
          map_set_table_1074047984_key_1[11] = *(u8*)(hdr_2 + 3);
          bool success_0 = state->map_set_table_1074047984.put(map_set_table_1074047984_key_1);
          // EP node  2904
          // BDD node 150:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__149))
          if ((0) == (success_0)) {
            // EP node  2905
            // BDD node 150:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__149))
            // EP node  3343
            // BDD node 151:vector_borrow(vector:(w64 1074110176), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074226080)[ -> (w64 1074124072)])
            buffer_t value_1;
            state->vector_table_1074110176.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_1);
            // EP node  3457
            // BDD node 152:vector_return(vector:(w64 1074110176), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124072)[(ReadLSB w16 (w32 0) vector_data__151)])
            // EP node  3867
            // BDD node 156:FORWARD
            cpu_hdr->egress_dev = bswap16((u16)value_1.get(0, 2));
          } else {
            // EP node  2906
            // BDD node 150:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space__149))
            // EP node  2957
            // BDD node 160:vector_borrow(vector:(w64 1074110176), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074228240)[ -> (w64 1074124072)])
            buffer_t value_2;
            state->vector_table_1074110176.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 65535), value_2);
            // EP node  3011
            // BDD node 161:vector_return(vector:(w64 1074110176), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074124072)[(ReadLSB w16 (w32 0) vector_data__160)])
            // EP node  3286
            // BDD node 165:FORWARD
            cpu_hdr->egress_dev = bswap16((u16)value_2.get(0, 2));
          }
        } else {
          // EP node  2807
          // BDD node 148:if ((Ule (ReadLSB w32 (w32 0) min_estimate__147) (w32 131071))
          // EP node  3631
          // BDD node 169:DROP
          result.forward = false;
        }
      } else {
        // EP node  2177
        // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
        // EP node  2178
        // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  2173
      // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
      // EP node  2174
      // BDD node 147:cms_count_min(cms:(w64 1074080384), key:(w64 1074225314)[(ReadLSB w64 (w32 268) packet_chunks) -> (ReadLSB w64 (w32 268) packet_chunks)])
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
