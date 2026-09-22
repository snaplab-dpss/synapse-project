#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  Meter tb_140;
  VectorTable vector_table_1074054008;
  VectorTable vector_table_1074071224;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      tb_140("tb_140","Ingress.tb_140",17179869184ULL,131072ULL,1000LL),
      vector_table_1074054008("vector_table_1074054008",{"Ingress.vector_table_1074054008_137",}),
      vector_table_1074071224("vector_table_1074071224",{"Ingress.vector_table_1074071224_175","Ingress.vector_table_1074071224_167","Ingress.vector_table_1074071224_159",})
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
  state->ingress_port_to_nf_dev.add_cpu_entry(asic_get_cpu_port());

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
  // BDD node 0:tb_allocate(capacity:(w32 65536), rate:(w64 17179869184), burst:(w64 131072), key_size:(w32 4), tb_out:(w64 1074041072)[(w64 0) -> (w64 1074041392)])
  // Module DataplaneMeterAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074041080)[(w64 0) -> (w64 1074054008)])
  // Module DataplaneVectorTableAllocate
  // BDD node 2:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074041088)[(w64 0) -> (w64 1074071224)])
  // Module DataplaneVectorTableAllocate
  // BDD node 3:vector_borrow(vector:(w64 1074054008), index:(w32 0), val_out:(w64 1074040944)[ -> (w64 1074067904)])
  // Module Ignore
  // BDD node 4:vector_return(vector:(w64 1074054008), index:(w32 0), value:(w64 1074067904)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_0(4);
  vector_table_1074054008_value_0.set(0, 4, 1);
  state->vector_table_1074054008.write(0, vector_table_1074054008_value_0);
  // BDD node 5:vector_borrow(vector:(w64 1074071224), index:(w32 0), val_out:(w64 1074041008)[ -> (w64 1074085120)])
  // Module Ignore
  // BDD node 6:vector_return(vector:(w64 1074071224), index:(w32 0), value:(w64 1074085120)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_0(2);
  vector_table_1074071224_value_0.set(0, 2, 1);
  state->vector_table_1074071224.write(0, vector_table_1074071224_value_0);
  // BDD node 7:vector_borrow(vector:(w64 1074054008), index:(w32 1), val_out:(w64 1074040944)[ -> (w64 1074067928)])
  // Module Ignore
  // BDD node 8:vector_return(vector:(w64 1074054008), index:(w32 1), value:(w64 1074067928)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_1(4);
  vector_table_1074054008_value_1.set(0, 4, 0);
  state->vector_table_1074054008.write(1, vector_table_1074054008_value_1);
  // BDD node 9:vector_borrow(vector:(w64 1074071224), index:(w32 1), val_out:(w64 1074041008)[ -> (w64 1074085144)])
  // Module Ignore
  // BDD node 10:vector_return(vector:(w64 1074071224), index:(w32 1), value:(w64 1074085144)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_1(2);
  vector_table_1074071224_value_1.set(0, 2, 0);
  state->vector_table_1074071224.write(1, vector_table_1074071224_value_1);
  // BDD node 11:vector_borrow(vector:(w64 1074054008), index:(w32 2), val_out:(w64 1074040944)[ -> (w64 1074067952)])
  // Module Ignore
  // BDD node 12:vector_return(vector:(w64 1074054008), index:(w32 2), value:(w64 1074067952)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_2(4);
  vector_table_1074054008_value_2.set(0, 4, 1);
  state->vector_table_1074054008.write(2, vector_table_1074054008_value_2);
  // BDD node 13:vector_borrow(vector:(w64 1074071224), index:(w32 2), val_out:(w64 1074041008)[ -> (w64 1074085168)])
  // Module Ignore
  // BDD node 14:vector_return(vector:(w64 1074071224), index:(w32 2), value:(w64 1074085168)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_2(2);
  vector_table_1074071224_value_2.set(0, 2, 3);
  state->vector_table_1074071224.write(2, vector_table_1074071224_value_2);
  // BDD node 15:vector_borrow(vector:(w64 1074054008), index:(w32 3), val_out:(w64 1074040944)[ -> (w64 1074067976)])
  // Module Ignore
  // BDD node 16:vector_return(vector:(w64 1074054008), index:(w32 3), value:(w64 1074067976)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_3(4);
  vector_table_1074054008_value_3.set(0, 4, 0);
  state->vector_table_1074054008.write(3, vector_table_1074054008_value_3);
  // BDD node 17:vector_borrow(vector:(w64 1074071224), index:(w32 3), val_out:(w64 1074041008)[ -> (w64 1074085192)])
  // Module Ignore
  // BDD node 18:vector_return(vector:(w64 1074071224), index:(w32 3), value:(w64 1074085192)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_3(2);
  vector_table_1074071224_value_3.set(0, 2, 2);
  state->vector_table_1074071224.write(3, vector_table_1074071224_value_3);
  // BDD node 19:vector_borrow(vector:(w64 1074054008), index:(w32 4), val_out:(w64 1074040944)[ -> (w64 1074068000)])
  // Module Ignore
  // BDD node 20:vector_return(vector:(w64 1074054008), index:(w32 4), value:(w64 1074068000)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_4(4);
  vector_table_1074054008_value_4.set(0, 4, 1);
  state->vector_table_1074054008.write(4, vector_table_1074054008_value_4);
  // BDD node 21:vector_borrow(vector:(w64 1074071224), index:(w32 4), val_out:(w64 1074041008)[ -> (w64 1074085216)])
  // Module Ignore
  // BDD node 22:vector_return(vector:(w64 1074071224), index:(w32 4), value:(w64 1074085216)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_4(2);
  vector_table_1074071224_value_4.set(0, 2, 5);
  state->vector_table_1074071224.write(4, vector_table_1074071224_value_4);
  // BDD node 23:vector_borrow(vector:(w64 1074054008), index:(w32 5), val_out:(w64 1074040944)[ -> (w64 1074068024)])
  // Module Ignore
  // BDD node 24:vector_return(vector:(w64 1074054008), index:(w32 5), value:(w64 1074068024)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_5(4);
  vector_table_1074054008_value_5.set(0, 4, 0);
  state->vector_table_1074054008.write(5, vector_table_1074054008_value_5);
  // BDD node 25:vector_borrow(vector:(w64 1074071224), index:(w32 5), val_out:(w64 1074041008)[ -> (w64 1074085240)])
  // Module Ignore
  // BDD node 26:vector_return(vector:(w64 1074071224), index:(w32 5), value:(w64 1074085240)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_5(2);
  vector_table_1074071224_value_5.set(0, 2, 4);
  state->vector_table_1074071224.write(5, vector_table_1074071224_value_5);
  // BDD node 27:vector_borrow(vector:(w64 1074054008), index:(w32 6), val_out:(w64 1074040944)[ -> (w64 1074068048)])
  // Module Ignore
  // BDD node 28:vector_return(vector:(w64 1074054008), index:(w32 6), value:(w64 1074068048)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_6(4);
  vector_table_1074054008_value_6.set(0, 4, 1);
  state->vector_table_1074054008.write(6, vector_table_1074054008_value_6);
  // BDD node 29:vector_borrow(vector:(w64 1074071224), index:(w32 6), val_out:(w64 1074041008)[ -> (w64 1074085264)])
  // Module Ignore
  // BDD node 30:vector_return(vector:(w64 1074071224), index:(w32 6), value:(w64 1074085264)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_6(2);
  vector_table_1074071224_value_6.set(0, 2, 7);
  state->vector_table_1074071224.write(6, vector_table_1074071224_value_6);
  // BDD node 31:vector_borrow(vector:(w64 1074054008), index:(w32 7), val_out:(w64 1074040944)[ -> (w64 1074068072)])
  // Module Ignore
  // BDD node 32:vector_return(vector:(w64 1074054008), index:(w32 7), value:(w64 1074068072)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_7(4);
  vector_table_1074054008_value_7.set(0, 4, 0);
  state->vector_table_1074054008.write(7, vector_table_1074054008_value_7);
  // BDD node 33:vector_borrow(vector:(w64 1074071224), index:(w32 7), val_out:(w64 1074041008)[ -> (w64 1074085288)])
  // Module Ignore
  // BDD node 34:vector_return(vector:(w64 1074071224), index:(w32 7), value:(w64 1074085288)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_7(2);
  vector_table_1074071224_value_7.set(0, 2, 6);
  state->vector_table_1074071224.write(7, vector_table_1074071224_value_7);
  // BDD node 35:vector_borrow(vector:(w64 1074054008), index:(w32 8), val_out:(w64 1074040944)[ -> (w64 1074068096)])
  // Module Ignore
  // BDD node 36:vector_return(vector:(w64 1074054008), index:(w32 8), value:(w64 1074068096)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_8(4);
  vector_table_1074054008_value_8.set(0, 4, 1);
  state->vector_table_1074054008.write(8, vector_table_1074054008_value_8);
  // BDD node 37:vector_borrow(vector:(w64 1074071224), index:(w32 8), val_out:(w64 1074041008)[ -> (w64 1074085312)])
  // Module Ignore
  // BDD node 38:vector_return(vector:(w64 1074071224), index:(w32 8), value:(w64 1074085312)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_8(2);
  vector_table_1074071224_value_8.set(0, 2, 9);
  state->vector_table_1074071224.write(8, vector_table_1074071224_value_8);
  // BDD node 39:vector_borrow(vector:(w64 1074054008), index:(w32 9), val_out:(w64 1074040944)[ -> (w64 1074068120)])
  // Module Ignore
  // BDD node 40:vector_return(vector:(w64 1074054008), index:(w32 9), value:(w64 1074068120)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_9(4);
  vector_table_1074054008_value_9.set(0, 4, 0);
  state->vector_table_1074054008.write(9, vector_table_1074054008_value_9);
  // BDD node 41:vector_borrow(vector:(w64 1074071224), index:(w32 9), val_out:(w64 1074041008)[ -> (w64 1074085336)])
  // Module Ignore
  // BDD node 42:vector_return(vector:(w64 1074071224), index:(w32 9), value:(w64 1074085336)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_9(2);
  vector_table_1074071224_value_9.set(0, 2, 8);
  state->vector_table_1074071224.write(9, vector_table_1074071224_value_9);
  // BDD node 43:vector_borrow(vector:(w64 1074054008), index:(w32 10), val_out:(w64 1074040944)[ -> (w64 1074068144)])
  // Module Ignore
  // BDD node 44:vector_return(vector:(w64 1074054008), index:(w32 10), value:(w64 1074068144)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_10(4);
  vector_table_1074054008_value_10.set(0, 4, 1);
  state->vector_table_1074054008.write(10, vector_table_1074054008_value_10);
  // BDD node 45:vector_borrow(vector:(w64 1074071224), index:(w32 10), val_out:(w64 1074041008)[ -> (w64 1074085360)])
  // Module Ignore
  // BDD node 46:vector_return(vector:(w64 1074071224), index:(w32 10), value:(w64 1074085360)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_10(2);
  vector_table_1074071224_value_10.set(0, 2, 11);
  state->vector_table_1074071224.write(10, vector_table_1074071224_value_10);
  // BDD node 47:vector_borrow(vector:(w64 1074054008), index:(w32 11), val_out:(w64 1074040944)[ -> (w64 1074068168)])
  // Module Ignore
  // BDD node 48:vector_return(vector:(w64 1074054008), index:(w32 11), value:(w64 1074068168)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_11(4);
  vector_table_1074054008_value_11.set(0, 4, 0);
  state->vector_table_1074054008.write(11, vector_table_1074054008_value_11);
  // BDD node 49:vector_borrow(vector:(w64 1074071224), index:(w32 11), val_out:(w64 1074041008)[ -> (w64 1074085384)])
  // Module Ignore
  // BDD node 50:vector_return(vector:(w64 1074071224), index:(w32 11), value:(w64 1074085384)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_11(2);
  vector_table_1074071224_value_11.set(0, 2, 10);
  state->vector_table_1074071224.write(11, vector_table_1074071224_value_11);
  // BDD node 51:vector_borrow(vector:(w64 1074054008), index:(w32 12), val_out:(w64 1074040944)[ -> (w64 1074068192)])
  // Module Ignore
  // BDD node 52:vector_return(vector:(w64 1074054008), index:(w32 12), value:(w64 1074068192)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_12(4);
  vector_table_1074054008_value_12.set(0, 4, 1);
  state->vector_table_1074054008.write(12, vector_table_1074054008_value_12);
  // BDD node 53:vector_borrow(vector:(w64 1074071224), index:(w32 12), val_out:(w64 1074041008)[ -> (w64 1074085408)])
  // Module Ignore
  // BDD node 54:vector_return(vector:(w64 1074071224), index:(w32 12), value:(w64 1074085408)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_12(2);
  vector_table_1074071224_value_12.set(0, 2, 13);
  state->vector_table_1074071224.write(12, vector_table_1074071224_value_12);
  // BDD node 55:vector_borrow(vector:(w64 1074054008), index:(w32 13), val_out:(w64 1074040944)[ -> (w64 1074068216)])
  // Module Ignore
  // BDD node 56:vector_return(vector:(w64 1074054008), index:(w32 13), value:(w64 1074068216)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_13(4);
  vector_table_1074054008_value_13.set(0, 4, 0);
  state->vector_table_1074054008.write(13, vector_table_1074054008_value_13);
  // BDD node 57:vector_borrow(vector:(w64 1074071224), index:(w32 13), val_out:(w64 1074041008)[ -> (w64 1074085432)])
  // Module Ignore
  // BDD node 58:vector_return(vector:(w64 1074071224), index:(w32 13), value:(w64 1074085432)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_13(2);
  vector_table_1074071224_value_13.set(0, 2, 12);
  state->vector_table_1074071224.write(13, vector_table_1074071224_value_13);
  // BDD node 59:vector_borrow(vector:(w64 1074054008), index:(w32 14), val_out:(w64 1074040944)[ -> (w64 1074068240)])
  // Module Ignore
  // BDD node 60:vector_return(vector:(w64 1074054008), index:(w32 14), value:(w64 1074068240)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_14(4);
  vector_table_1074054008_value_14.set(0, 4, 1);
  state->vector_table_1074054008.write(14, vector_table_1074054008_value_14);
  // BDD node 61:vector_borrow(vector:(w64 1074071224), index:(w32 14), val_out:(w64 1074041008)[ -> (w64 1074085456)])
  // Module Ignore
  // BDD node 62:vector_return(vector:(w64 1074071224), index:(w32 14), value:(w64 1074085456)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_14(2);
  vector_table_1074071224_value_14.set(0, 2, 15);
  state->vector_table_1074071224.write(14, vector_table_1074071224_value_14);
  // BDD node 63:vector_borrow(vector:(w64 1074054008), index:(w32 15), val_out:(w64 1074040944)[ -> (w64 1074068264)])
  // Module Ignore
  // BDD node 64:vector_return(vector:(w64 1074054008), index:(w32 15), value:(w64 1074068264)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_15(4);
  vector_table_1074054008_value_15.set(0, 4, 0);
  state->vector_table_1074054008.write(15, vector_table_1074054008_value_15);
  // BDD node 65:vector_borrow(vector:(w64 1074071224), index:(w32 15), val_out:(w64 1074041008)[ -> (w64 1074085480)])
  // Module Ignore
  // BDD node 66:vector_return(vector:(w64 1074071224), index:(w32 15), value:(w64 1074085480)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_15(2);
  vector_table_1074071224_value_15.set(0, 2, 14);
  state->vector_table_1074071224.write(15, vector_table_1074071224_value_15);
  // BDD node 67:vector_borrow(vector:(w64 1074054008), index:(w32 16), val_out:(w64 1074040944)[ -> (w64 1074068288)])
  // Module Ignore
  // BDD node 68:vector_return(vector:(w64 1074054008), index:(w32 16), value:(w64 1074068288)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_16(4);
  vector_table_1074054008_value_16.set(0, 4, 1);
  state->vector_table_1074054008.write(16, vector_table_1074054008_value_16);
  // BDD node 69:vector_borrow(vector:(w64 1074071224), index:(w32 16), val_out:(w64 1074041008)[ -> (w64 1074085504)])
  // Module Ignore
  // BDD node 70:vector_return(vector:(w64 1074071224), index:(w32 16), value:(w64 1074085504)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_16(2);
  vector_table_1074071224_value_16.set(0, 2, 17);
  state->vector_table_1074071224.write(16, vector_table_1074071224_value_16);
  // BDD node 71:vector_borrow(vector:(w64 1074054008), index:(w32 17), val_out:(w64 1074040944)[ -> (w64 1074068312)])
  // Module Ignore
  // BDD node 72:vector_return(vector:(w64 1074054008), index:(w32 17), value:(w64 1074068312)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_17(4);
  vector_table_1074054008_value_17.set(0, 4, 0);
  state->vector_table_1074054008.write(17, vector_table_1074054008_value_17);
  // BDD node 73:vector_borrow(vector:(w64 1074071224), index:(w32 17), val_out:(w64 1074041008)[ -> (w64 1074085528)])
  // Module Ignore
  // BDD node 74:vector_return(vector:(w64 1074071224), index:(w32 17), value:(w64 1074085528)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_17(2);
  vector_table_1074071224_value_17.set(0, 2, 16);
  state->vector_table_1074071224.write(17, vector_table_1074071224_value_17);
  // BDD node 75:vector_borrow(vector:(w64 1074054008), index:(w32 18), val_out:(w64 1074040944)[ -> (w64 1074068336)])
  // Module Ignore
  // BDD node 76:vector_return(vector:(w64 1074054008), index:(w32 18), value:(w64 1074068336)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_18(4);
  vector_table_1074054008_value_18.set(0, 4, 1);
  state->vector_table_1074054008.write(18, vector_table_1074054008_value_18);
  // BDD node 77:vector_borrow(vector:(w64 1074071224), index:(w32 18), val_out:(w64 1074041008)[ -> (w64 1074085552)])
  // Module Ignore
  // BDD node 78:vector_return(vector:(w64 1074071224), index:(w32 18), value:(w64 1074085552)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_18(2);
  vector_table_1074071224_value_18.set(0, 2, 19);
  state->vector_table_1074071224.write(18, vector_table_1074071224_value_18);
  // BDD node 79:vector_borrow(vector:(w64 1074054008), index:(w32 19), val_out:(w64 1074040944)[ -> (w64 1074068360)])
  // Module Ignore
  // BDD node 80:vector_return(vector:(w64 1074054008), index:(w32 19), value:(w64 1074068360)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_19(4);
  vector_table_1074054008_value_19.set(0, 4, 0);
  state->vector_table_1074054008.write(19, vector_table_1074054008_value_19);
  // BDD node 81:vector_borrow(vector:(w64 1074071224), index:(w32 19), val_out:(w64 1074041008)[ -> (w64 1074085576)])
  // Module Ignore
  // BDD node 82:vector_return(vector:(w64 1074071224), index:(w32 19), value:(w64 1074085576)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_19(2);
  vector_table_1074071224_value_19.set(0, 2, 18);
  state->vector_table_1074071224.write(19, vector_table_1074071224_value_19);
  // BDD node 83:vector_borrow(vector:(w64 1074054008), index:(w32 20), val_out:(w64 1074040944)[ -> (w64 1074068384)])
  // Module Ignore
  // BDD node 84:vector_return(vector:(w64 1074054008), index:(w32 20), value:(w64 1074068384)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_20(4);
  vector_table_1074054008_value_20.set(0, 4, 1);
  state->vector_table_1074054008.write(20, vector_table_1074054008_value_20);
  // BDD node 85:vector_borrow(vector:(w64 1074071224), index:(w32 20), val_out:(w64 1074041008)[ -> (w64 1074085600)])
  // Module Ignore
  // BDD node 86:vector_return(vector:(w64 1074071224), index:(w32 20), value:(w64 1074085600)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_20(2);
  vector_table_1074071224_value_20.set(0, 2, 21);
  state->vector_table_1074071224.write(20, vector_table_1074071224_value_20);
  // BDD node 87:vector_borrow(vector:(w64 1074054008), index:(w32 21), val_out:(w64 1074040944)[ -> (w64 1074068408)])
  // Module Ignore
  // BDD node 88:vector_return(vector:(w64 1074054008), index:(w32 21), value:(w64 1074068408)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_21(4);
  vector_table_1074054008_value_21.set(0, 4, 0);
  state->vector_table_1074054008.write(21, vector_table_1074054008_value_21);
  // BDD node 89:vector_borrow(vector:(w64 1074071224), index:(w32 21), val_out:(w64 1074041008)[ -> (w64 1074085624)])
  // Module Ignore
  // BDD node 90:vector_return(vector:(w64 1074071224), index:(w32 21), value:(w64 1074085624)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_21(2);
  vector_table_1074071224_value_21.set(0, 2, 20);
  state->vector_table_1074071224.write(21, vector_table_1074071224_value_21);
  // BDD node 91:vector_borrow(vector:(w64 1074054008), index:(w32 22), val_out:(w64 1074040944)[ -> (w64 1074068432)])
  // Module Ignore
  // BDD node 92:vector_return(vector:(w64 1074054008), index:(w32 22), value:(w64 1074068432)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_22(4);
  vector_table_1074054008_value_22.set(0, 4, 1);
  state->vector_table_1074054008.write(22, vector_table_1074054008_value_22);
  // BDD node 93:vector_borrow(vector:(w64 1074071224), index:(w32 22), val_out:(w64 1074041008)[ -> (w64 1074085648)])
  // Module Ignore
  // BDD node 94:vector_return(vector:(w64 1074071224), index:(w32 22), value:(w64 1074085648)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_22(2);
  vector_table_1074071224_value_22.set(0, 2, 23);
  state->vector_table_1074071224.write(22, vector_table_1074071224_value_22);
  // BDD node 95:vector_borrow(vector:(w64 1074054008), index:(w32 23), val_out:(w64 1074040944)[ -> (w64 1074068456)])
  // Module Ignore
  // BDD node 96:vector_return(vector:(w64 1074054008), index:(w32 23), value:(w64 1074068456)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_23(4);
  vector_table_1074054008_value_23.set(0, 4, 0);
  state->vector_table_1074054008.write(23, vector_table_1074054008_value_23);
  // BDD node 97:vector_borrow(vector:(w64 1074071224), index:(w32 23), val_out:(w64 1074041008)[ -> (w64 1074085672)])
  // Module Ignore
  // BDD node 98:vector_return(vector:(w64 1074071224), index:(w32 23), value:(w64 1074085672)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_23(2);
  vector_table_1074071224_value_23.set(0, 2, 22);
  state->vector_table_1074071224.write(23, vector_table_1074071224_value_23);
  // BDD node 99:vector_borrow(vector:(w64 1074054008), index:(w32 24), val_out:(w64 1074040944)[ -> (w64 1074068480)])
  // Module Ignore
  // BDD node 100:vector_return(vector:(w64 1074054008), index:(w32 24), value:(w64 1074068480)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_24(4);
  vector_table_1074054008_value_24.set(0, 4, 1);
  state->vector_table_1074054008.write(24, vector_table_1074054008_value_24);
  // BDD node 101:vector_borrow(vector:(w64 1074071224), index:(w32 24), val_out:(w64 1074041008)[ -> (w64 1074085696)])
  // Module Ignore
  // BDD node 102:vector_return(vector:(w64 1074071224), index:(w32 24), value:(w64 1074085696)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_24(2);
  vector_table_1074071224_value_24.set(0, 2, 25);
  state->vector_table_1074071224.write(24, vector_table_1074071224_value_24);
  // BDD node 103:vector_borrow(vector:(w64 1074054008), index:(w32 25), val_out:(w64 1074040944)[ -> (w64 1074068504)])
  // Module Ignore
  // BDD node 104:vector_return(vector:(w64 1074054008), index:(w32 25), value:(w64 1074068504)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_25(4);
  vector_table_1074054008_value_25.set(0, 4, 0);
  state->vector_table_1074054008.write(25, vector_table_1074054008_value_25);
  // BDD node 105:vector_borrow(vector:(w64 1074071224), index:(w32 25), val_out:(w64 1074041008)[ -> (w64 1074085720)])
  // Module Ignore
  // BDD node 106:vector_return(vector:(w64 1074071224), index:(w32 25), value:(w64 1074085720)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_25(2);
  vector_table_1074071224_value_25.set(0, 2, 24);
  state->vector_table_1074071224.write(25, vector_table_1074071224_value_25);
  // BDD node 107:vector_borrow(vector:(w64 1074054008), index:(w32 26), val_out:(w64 1074040944)[ -> (w64 1074068528)])
  // Module Ignore
  // BDD node 108:vector_return(vector:(w64 1074054008), index:(w32 26), value:(w64 1074068528)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_26(4);
  vector_table_1074054008_value_26.set(0, 4, 1);
  state->vector_table_1074054008.write(26, vector_table_1074054008_value_26);
  // BDD node 109:vector_borrow(vector:(w64 1074071224), index:(w32 26), val_out:(w64 1074041008)[ -> (w64 1074085744)])
  // Module Ignore
  // BDD node 110:vector_return(vector:(w64 1074071224), index:(w32 26), value:(w64 1074085744)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_26(2);
  vector_table_1074071224_value_26.set(0, 2, 27);
  state->vector_table_1074071224.write(26, vector_table_1074071224_value_26);
  // BDD node 111:vector_borrow(vector:(w64 1074054008), index:(w32 27), val_out:(w64 1074040944)[ -> (w64 1074068552)])
  // Module Ignore
  // BDD node 112:vector_return(vector:(w64 1074054008), index:(w32 27), value:(w64 1074068552)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_27(4);
  vector_table_1074054008_value_27.set(0, 4, 0);
  state->vector_table_1074054008.write(27, vector_table_1074054008_value_27);
  // BDD node 113:vector_borrow(vector:(w64 1074071224), index:(w32 27), val_out:(w64 1074041008)[ -> (w64 1074085768)])
  // Module Ignore
  // BDD node 114:vector_return(vector:(w64 1074071224), index:(w32 27), value:(w64 1074085768)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_27(2);
  vector_table_1074071224_value_27.set(0, 2, 26);
  state->vector_table_1074071224.write(27, vector_table_1074071224_value_27);
  // BDD node 115:vector_borrow(vector:(w64 1074054008), index:(w32 28), val_out:(w64 1074040944)[ -> (w64 1074068576)])
  // Module Ignore
  // BDD node 116:vector_return(vector:(w64 1074054008), index:(w32 28), value:(w64 1074068576)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_28(4);
  vector_table_1074054008_value_28.set(0, 4, 1);
  state->vector_table_1074054008.write(28, vector_table_1074054008_value_28);
  // BDD node 117:vector_borrow(vector:(w64 1074071224), index:(w32 28), val_out:(w64 1074041008)[ -> (w64 1074085792)])
  // Module Ignore
  // BDD node 118:vector_return(vector:(w64 1074071224), index:(w32 28), value:(w64 1074085792)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_28(2);
  vector_table_1074071224_value_28.set(0, 2, 29);
  state->vector_table_1074071224.write(28, vector_table_1074071224_value_28);
  // BDD node 119:vector_borrow(vector:(w64 1074054008), index:(w32 29), val_out:(w64 1074040944)[ -> (w64 1074068600)])
  // Module Ignore
  // BDD node 120:vector_return(vector:(w64 1074054008), index:(w32 29), value:(w64 1074068600)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_29(4);
  vector_table_1074054008_value_29.set(0, 4, 0);
  state->vector_table_1074054008.write(29, vector_table_1074054008_value_29);
  // BDD node 121:vector_borrow(vector:(w64 1074071224), index:(w32 29), val_out:(w64 1074041008)[ -> (w64 1074085816)])
  // Module Ignore
  // BDD node 122:vector_return(vector:(w64 1074071224), index:(w32 29), value:(w64 1074085816)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_29(2);
  vector_table_1074071224_value_29.set(0, 2, 28);
  state->vector_table_1074071224.write(29, vector_table_1074071224_value_29);
  // BDD node 123:vector_borrow(vector:(w64 1074054008), index:(w32 30), val_out:(w64 1074040944)[ -> (w64 1074068624)])
  // Module Ignore
  // BDD node 124:vector_return(vector:(w64 1074054008), index:(w32 30), value:(w64 1074068624)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_30(4);
  vector_table_1074054008_value_30.set(0, 4, 1);
  state->vector_table_1074054008.write(30, vector_table_1074054008_value_30);
  // BDD node 125:vector_borrow(vector:(w64 1074071224), index:(w32 30), val_out:(w64 1074041008)[ -> (w64 1074085840)])
  // Module Ignore
  // BDD node 126:vector_return(vector:(w64 1074071224), index:(w32 30), value:(w64 1074085840)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_30(2);
  vector_table_1074071224_value_30.set(0, 2, 31);
  state->vector_table_1074071224.write(30, vector_table_1074071224_value_30);
  // BDD node 127:vector_borrow(vector:(w64 1074054008), index:(w32 31), val_out:(w64 1074040944)[ -> (w64 1074068648)])
  // Module Ignore
  // BDD node 128:vector_return(vector:(w64 1074054008), index:(w32 31), value:(w64 1074068648)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074054008_value_31(4);
  vector_table_1074054008_value_31.set(0, 4, 0);
  state->vector_table_1074054008.write(31, vector_table_1074054008_value_31);
  // BDD node 129:vector_borrow(vector:(w64 1074071224), index:(w32 31), val_out:(w64 1074041008)[ -> (w64 1074085864)])
  // Module Ignore
  // BDD node 130:vector_return(vector:(w64 1074071224), index:(w32 31), value:(w64 1074085864)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074071224_value_31(2);
  vector_table_1074071224_value_31.set(0, 2, 30);
  state->vector_table_1074071224.write(31, vector_table_1074071224_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 time; // The switch's clock at the hand-off, ingress_mac_tstamp[47:16].
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

  now = ((time_ns_t)bswap32(cpu_hdr_extra->time)) << 16;


  if (bswap16(cpu_hdr->code_path) == 0) {
    // EP node  2348
    // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  2349
    // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  2350
    // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  2351
    // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
    buffer_t value_0;
    state->vector_table_1074054008.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull), value_0);
    // EP node  2352
    // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  2353
      // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
      // EP node  2356
      // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
      buffer_t tb_140_key_0(4);
      tb_140_key_0[0] = *(u8*)(hdr_1 + 16);
      tb_140_key_0[1] = *(u8*)(hdr_1 + 17);
      tb_140_key_0[2] = *(u8*)(hdr_1 + 18);
      tb_140_key_0[3] = *(u8*)(hdr_1 + 19);
      bool is_tracing_0 = state->tb_140.is_tracking(tb_140_key_0);
      // EP node  2357
      // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
      if ((0) == (is_tracing_0)) {
        // EP node  2358
        // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
        // EP node  5145
        // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
        buffer_t tb_140_key_1(4);
        tb_140_key_1[0] = *(u8*)(hdr_1 + 16);
        tb_140_key_1[1] = *(u8*)(hdr_1 + 17);
        tb_140_key_1[2] = *(u8*)(hdr_1 + 18);
        tb_140_key_1[3] = *(u8*)(hdr_1 + 19);
        bool success_0 = state->tb_140.trace(tb_140_key_1);
        // EP node  5208
        // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) successfuly_tracing__142))
        if ((0) == (success_0)) {
          // EP node  5209
          // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) successfuly_tracing__142))
          // EP node  5678
          // BDD node 147:DROP
          result.forward = false;
        } else {
          // EP node  5210
          // BDD node 143:if ((Eq (w32 0) (ReadLSB w32 (w32 0) successfuly_tracing__142))
          // EP node  5340
          // BDD node 148:vector_borrow(vector:(w64 1074071224), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1074186456)[ -> (w64 1074085120)])
          buffer_t value_1;
          state->vector_table_1074071224.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull), value_1);
          // EP node  5474
          // BDD node 149:vector_return(vector:(w64 1074071224), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1074085120)[(ReadLSB w16 (w32 0) vector_data__148)])
          // EP node  5885
          // BDD node 153:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__148)))
          if ((bswap32(cpu_hdr_extra->DEVICE) & 0xffffull) != ((u16)value_1.get(0, 2))) {
            // EP node  5886
            // BDD node 153:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__148)))
            // EP node  6029
            // BDD node 154:FORWARD
            cpu_hdr->egress_dev = bswap16((u16)value_1.get(0, 2));
          } else {
            // EP node  5887
            // BDD node 153:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__148)))
            // EP node  6030
            // BDD node 155:DROP
            result.forward = false;
          }
        }
      } else {
        // EP node  2359
        // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
        // EP node  2360
        // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  2354
      // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
      // EP node  2355
      // BDD node 142:tb_trace(tb:(w64 1074041392), key:(w64 1073759344)[(ReadLSB w32 (w32 272) packet_chunks) -> (ReadLSB w32 (w32 272) packet_chunks)], pkt_len:(Extract w16 0 (ZExt w32 (ReadLSB w16 (w32 0) pkt_len))), time:(ReadLSB w64 (w32 0) next_time), index_out:(w64 1074182712)[(ReadLSB w32 (w32 0) index_out__140) -> (ReadLSB w32 (w32 0) index_out__142)])
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
