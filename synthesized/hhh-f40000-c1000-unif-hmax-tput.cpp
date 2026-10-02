#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  VectorRegister vector_register_1074038336;
  VectorRegister vector_register_1074055552;
  VectorRegister vector_register_1074072832;
  VectorRegister vector_register_1074091064;
  VectorRegister vector_register_1074109296;
  VectorRegister vector_register_1074127528;
  VectorRegister vector_register_1074145760;
  VectorRegister vector_register_1074163992;
  CRC32 crc32_hasher_1074182056;
  CRC32 crc32_hasher_1074183352;
  CRC32 crc32_hasher_1074184648;
  CRC32 crc32_hasher_1074185944;
  CRC32 crc32_hasher_1074187240;
  CRC32 crc32_hasher_1074188536;
  VectorTable vector_table_1074190072;
  VectorTable vector_table_1074207288;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      vector_register_1074038336("vector_register_1074038336",{"Ingress.vector_register_1074038336_0",}, 1000LL),
      vector_register_1074055552("vector_register_1074055552",{"Ingress.vector_register_1074055552_0",}, 1000LL),
      vector_register_1074072832("vector_register_1074072832",{"Ingress.vector_register_1074072832_0",}, 1000LL),
      vector_register_1074091064("vector_register_1074091064",{"Ingress.vector_register_1074091064_0",}, 1000LL),
      vector_register_1074109296("vector_register_1074109296",{"Ingress.vector_register_1074109296_0",}, 1000LL),
      vector_register_1074127528("vector_register_1074127528",{"Ingress.vector_register_1074127528_0",}, 1000LL),
      vector_register_1074145760("vector_register_1074145760",{"Ingress.vector_register_1074145760_0",}, 1000LL),
      vector_register_1074163992("vector_register_1074163992",{"Ingress.vector_register_1074163992_0",}, 1000LL),
      crc32_hasher_1074182056(libnf::crc32_config{"", 0x04c11db7, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1074183352(libnf::crc32_config{"", 0x7b17a39f, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1074184648(libnf::crc32_config{"", 0x99f29aad, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1074185944(libnf::crc32_config{"", 0x21bca2c3, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1074187240(libnf::crc32_config{"", 0x0c596849, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1074188536(libnf::crc32_config{"", 0x3553d717, true, 0xffffffff, 0xffffffff}),
      vector_table_1074190072("vector_table_1074190072",{"Ingress.vector_table_1074190072_157",}),
      vector_table_1074207288("vector_table_1074207288",{"Ingress.vector_table_1074207288_184","Ingress.vector_table_1074207288_176",})
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
  // BDD node 0:vector_allocate(elem_size:(w32 4), capacity:(w32 256), vector_out:(w64 1074038000)[(w64 0) -> (w64 1074038336)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 4), capacity:(w32 65536), vector_out:(w64 1074038008)[(w64 0) -> (w64 1074055552)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 2:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1074072520)[(w64 0) -> (w64 1074072832)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 3:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1074072528)[(w64 0) -> (w64 1074091064)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 4:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1074072536)[(w64 0) -> (w64 1074109296)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 5:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1074072544)[(w64 0) -> (w64 1074127528)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 6:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1074072552)[(w64 0) -> (w64 1074145760)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 7:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1074072560)[(w64 0) -> (w64 1074163992)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 8:crc32_hasher_init(hasher:(w64 1074182056), polynomial:(w32 79764919), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 9:crc32_hasher_init(hasher:(w64 1074183352), polynomial:(w32 2065146783), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 10:crc32_hasher_init(hasher:(w64 1074184648), polynomial:(w32 2582813357), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 11:crc32_hasher_init(hasher:(w64 1074185944), polynomial:(w32 566010563), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 12:crc32_hasher_init(hasher:(w64 1074187240), polynomial:(w32 207185993), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 13:crc32_hasher_init(hasher:(w64 1074188536), polynomial:(w32 894686999), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 14:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1074038032)[(w64 0) -> (w64 1074190072)])
  // Module DataplaneVectorTableAllocate
  // BDD node 15:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1074038040)[(w64 0) -> (w64 1074207288)])
  // Module DataplaneVectorTableAllocate
  // BDD node 16:vector_borrow(vector:(w64 1074190072), index:(w32 0), val_out:(w64 1074037872)[ -> (w64 1074203968)])
  // Module Ignore
  // BDD node 17:vector_return(vector:(w64 1074190072), index:(w32 0), value:(w64 1074203968)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_0(4);
  vector_table_1074190072_value_0.set(0, 4, 1);
  state->vector_table_1074190072.write(0, vector_table_1074190072_value_0);
  // BDD node 18:vector_borrow(vector:(w64 1074207288), index:(w32 0), val_out:(w64 1074037936)[ -> (w64 1074221184)])
  // Module Ignore
  // BDD node 19:vector_return(vector:(w64 1074207288), index:(w32 0), value:(w64 1074221184)[(w16 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_0(2);
  vector_table_1074207288_value_0.set(0, 2, 1);
  state->vector_table_1074207288.write(0, vector_table_1074207288_value_0);
  // BDD node 20:vector_borrow(vector:(w64 1074190072), index:(w32 1), val_out:(w64 1074037872)[ -> (w64 1074203992)])
  // Module Ignore
  // BDD node 21:vector_return(vector:(w64 1074190072), index:(w32 1), value:(w64 1074203992)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_1(4);
  vector_table_1074190072_value_1.set(0, 4, 0);
  state->vector_table_1074190072.write(1, vector_table_1074190072_value_1);
  // BDD node 22:vector_borrow(vector:(w64 1074207288), index:(w32 1), val_out:(w64 1074037936)[ -> (w64 1074221208)])
  // Module Ignore
  // BDD node 23:vector_return(vector:(w64 1074207288), index:(w32 1), value:(w64 1074221208)[(w16 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_1(2);
  vector_table_1074207288_value_1.set(0, 2, 0);
  state->vector_table_1074207288.write(1, vector_table_1074207288_value_1);
  // BDD node 24:vector_borrow(vector:(w64 1074190072), index:(w32 2), val_out:(w64 1074037872)[ -> (w64 1074204016)])
  // Module Ignore
  // BDD node 25:vector_return(vector:(w64 1074190072), index:(w32 2), value:(w64 1074204016)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_2(4);
  vector_table_1074190072_value_2.set(0, 4, 1);
  state->vector_table_1074190072.write(2, vector_table_1074190072_value_2);
  // BDD node 26:vector_borrow(vector:(w64 1074207288), index:(w32 2), val_out:(w64 1074037936)[ -> (w64 1074221232)])
  // Module Ignore
  // BDD node 27:vector_return(vector:(w64 1074207288), index:(w32 2), value:(w64 1074221232)[(w16 3)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_2(2);
  vector_table_1074207288_value_2.set(0, 2, 3);
  state->vector_table_1074207288.write(2, vector_table_1074207288_value_2);
  // BDD node 28:vector_borrow(vector:(w64 1074190072), index:(w32 3), val_out:(w64 1074037872)[ -> (w64 1074204040)])
  // Module Ignore
  // BDD node 29:vector_return(vector:(w64 1074190072), index:(w32 3), value:(w64 1074204040)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_3(4);
  vector_table_1074190072_value_3.set(0, 4, 0);
  state->vector_table_1074190072.write(3, vector_table_1074190072_value_3);
  // BDD node 30:vector_borrow(vector:(w64 1074207288), index:(w32 3), val_out:(w64 1074037936)[ -> (w64 1074221256)])
  // Module Ignore
  // BDD node 31:vector_return(vector:(w64 1074207288), index:(w32 3), value:(w64 1074221256)[(w16 2)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_3(2);
  vector_table_1074207288_value_3.set(0, 2, 2);
  state->vector_table_1074207288.write(3, vector_table_1074207288_value_3);
  // BDD node 32:vector_borrow(vector:(w64 1074190072), index:(w32 4), val_out:(w64 1074037872)[ -> (w64 1074204064)])
  // Module Ignore
  // BDD node 33:vector_return(vector:(w64 1074190072), index:(w32 4), value:(w64 1074204064)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_4(4);
  vector_table_1074190072_value_4.set(0, 4, 1);
  state->vector_table_1074190072.write(4, vector_table_1074190072_value_4);
  // BDD node 34:vector_borrow(vector:(w64 1074207288), index:(w32 4), val_out:(w64 1074037936)[ -> (w64 1074221280)])
  // Module Ignore
  // BDD node 35:vector_return(vector:(w64 1074207288), index:(w32 4), value:(w64 1074221280)[(w16 5)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_4(2);
  vector_table_1074207288_value_4.set(0, 2, 5);
  state->vector_table_1074207288.write(4, vector_table_1074207288_value_4);
  // BDD node 36:vector_borrow(vector:(w64 1074190072), index:(w32 5), val_out:(w64 1074037872)[ -> (w64 1074204088)])
  // Module Ignore
  // BDD node 37:vector_return(vector:(w64 1074190072), index:(w32 5), value:(w64 1074204088)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_5(4);
  vector_table_1074190072_value_5.set(0, 4, 0);
  state->vector_table_1074190072.write(5, vector_table_1074190072_value_5);
  // BDD node 38:vector_borrow(vector:(w64 1074207288), index:(w32 5), val_out:(w64 1074037936)[ -> (w64 1074221304)])
  // Module Ignore
  // BDD node 39:vector_return(vector:(w64 1074207288), index:(w32 5), value:(w64 1074221304)[(w16 4)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_5(2);
  vector_table_1074207288_value_5.set(0, 2, 4);
  state->vector_table_1074207288.write(5, vector_table_1074207288_value_5);
  // BDD node 40:vector_borrow(vector:(w64 1074190072), index:(w32 6), val_out:(w64 1074037872)[ -> (w64 1074204112)])
  // Module Ignore
  // BDD node 41:vector_return(vector:(w64 1074190072), index:(w32 6), value:(w64 1074204112)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_6(4);
  vector_table_1074190072_value_6.set(0, 4, 1);
  state->vector_table_1074190072.write(6, vector_table_1074190072_value_6);
  // BDD node 42:vector_borrow(vector:(w64 1074207288), index:(w32 6), val_out:(w64 1074037936)[ -> (w64 1074221328)])
  // Module Ignore
  // BDD node 43:vector_return(vector:(w64 1074207288), index:(w32 6), value:(w64 1074221328)[(w16 7)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_6(2);
  vector_table_1074207288_value_6.set(0, 2, 7);
  state->vector_table_1074207288.write(6, vector_table_1074207288_value_6);
  // BDD node 44:vector_borrow(vector:(w64 1074190072), index:(w32 7), val_out:(w64 1074037872)[ -> (w64 1074204136)])
  // Module Ignore
  // BDD node 45:vector_return(vector:(w64 1074190072), index:(w32 7), value:(w64 1074204136)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_7(4);
  vector_table_1074190072_value_7.set(0, 4, 0);
  state->vector_table_1074190072.write(7, vector_table_1074190072_value_7);
  // BDD node 46:vector_borrow(vector:(w64 1074207288), index:(w32 7), val_out:(w64 1074037936)[ -> (w64 1074221352)])
  // Module Ignore
  // BDD node 47:vector_return(vector:(w64 1074207288), index:(w32 7), value:(w64 1074221352)[(w16 6)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_7(2);
  vector_table_1074207288_value_7.set(0, 2, 6);
  state->vector_table_1074207288.write(7, vector_table_1074207288_value_7);
  // BDD node 48:vector_borrow(vector:(w64 1074190072), index:(w32 8), val_out:(w64 1074037872)[ -> (w64 1074204160)])
  // Module Ignore
  // BDD node 49:vector_return(vector:(w64 1074190072), index:(w32 8), value:(w64 1074204160)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_8(4);
  vector_table_1074190072_value_8.set(0, 4, 1);
  state->vector_table_1074190072.write(8, vector_table_1074190072_value_8);
  // BDD node 50:vector_borrow(vector:(w64 1074207288), index:(w32 8), val_out:(w64 1074037936)[ -> (w64 1074221376)])
  // Module Ignore
  // BDD node 51:vector_return(vector:(w64 1074207288), index:(w32 8), value:(w64 1074221376)[(w16 9)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_8(2);
  vector_table_1074207288_value_8.set(0, 2, 9);
  state->vector_table_1074207288.write(8, vector_table_1074207288_value_8);
  // BDD node 52:vector_borrow(vector:(w64 1074190072), index:(w32 9), val_out:(w64 1074037872)[ -> (w64 1074204184)])
  // Module Ignore
  // BDD node 53:vector_return(vector:(w64 1074190072), index:(w32 9), value:(w64 1074204184)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_9(4);
  vector_table_1074190072_value_9.set(0, 4, 0);
  state->vector_table_1074190072.write(9, vector_table_1074190072_value_9);
  // BDD node 54:vector_borrow(vector:(w64 1074207288), index:(w32 9), val_out:(w64 1074037936)[ -> (w64 1074221400)])
  // Module Ignore
  // BDD node 55:vector_return(vector:(w64 1074207288), index:(w32 9), value:(w64 1074221400)[(w16 8)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_9(2);
  vector_table_1074207288_value_9.set(0, 2, 8);
  state->vector_table_1074207288.write(9, vector_table_1074207288_value_9);
  // BDD node 56:vector_borrow(vector:(w64 1074190072), index:(w32 10), val_out:(w64 1074037872)[ -> (w64 1074204208)])
  // Module Ignore
  // BDD node 57:vector_return(vector:(w64 1074190072), index:(w32 10), value:(w64 1074204208)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_10(4);
  vector_table_1074190072_value_10.set(0, 4, 1);
  state->vector_table_1074190072.write(10, vector_table_1074190072_value_10);
  // BDD node 58:vector_borrow(vector:(w64 1074207288), index:(w32 10), val_out:(w64 1074037936)[ -> (w64 1074221424)])
  // Module Ignore
  // BDD node 59:vector_return(vector:(w64 1074207288), index:(w32 10), value:(w64 1074221424)[(w16 11)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_10(2);
  vector_table_1074207288_value_10.set(0, 2, 11);
  state->vector_table_1074207288.write(10, vector_table_1074207288_value_10);
  // BDD node 60:vector_borrow(vector:(w64 1074190072), index:(w32 11), val_out:(w64 1074037872)[ -> (w64 1074204232)])
  // Module Ignore
  // BDD node 61:vector_return(vector:(w64 1074190072), index:(w32 11), value:(w64 1074204232)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_11(4);
  vector_table_1074190072_value_11.set(0, 4, 0);
  state->vector_table_1074190072.write(11, vector_table_1074190072_value_11);
  // BDD node 62:vector_borrow(vector:(w64 1074207288), index:(w32 11), val_out:(w64 1074037936)[ -> (w64 1074221448)])
  // Module Ignore
  // BDD node 63:vector_return(vector:(w64 1074207288), index:(w32 11), value:(w64 1074221448)[(w16 10)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_11(2);
  vector_table_1074207288_value_11.set(0, 2, 10);
  state->vector_table_1074207288.write(11, vector_table_1074207288_value_11);
  // BDD node 64:vector_borrow(vector:(w64 1074190072), index:(w32 12), val_out:(w64 1074037872)[ -> (w64 1074204256)])
  // Module Ignore
  // BDD node 65:vector_return(vector:(w64 1074190072), index:(w32 12), value:(w64 1074204256)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_12(4);
  vector_table_1074190072_value_12.set(0, 4, 1);
  state->vector_table_1074190072.write(12, vector_table_1074190072_value_12);
  // BDD node 66:vector_borrow(vector:(w64 1074207288), index:(w32 12), val_out:(w64 1074037936)[ -> (w64 1074221472)])
  // Module Ignore
  // BDD node 67:vector_return(vector:(w64 1074207288), index:(w32 12), value:(w64 1074221472)[(w16 13)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_12(2);
  vector_table_1074207288_value_12.set(0, 2, 13);
  state->vector_table_1074207288.write(12, vector_table_1074207288_value_12);
  // BDD node 68:vector_borrow(vector:(w64 1074190072), index:(w32 13), val_out:(w64 1074037872)[ -> (w64 1074204280)])
  // Module Ignore
  // BDD node 69:vector_return(vector:(w64 1074190072), index:(w32 13), value:(w64 1074204280)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_13(4);
  vector_table_1074190072_value_13.set(0, 4, 0);
  state->vector_table_1074190072.write(13, vector_table_1074190072_value_13);
  // BDD node 70:vector_borrow(vector:(w64 1074207288), index:(w32 13), val_out:(w64 1074037936)[ -> (w64 1074221496)])
  // Module Ignore
  // BDD node 71:vector_return(vector:(w64 1074207288), index:(w32 13), value:(w64 1074221496)[(w16 12)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_13(2);
  vector_table_1074207288_value_13.set(0, 2, 12);
  state->vector_table_1074207288.write(13, vector_table_1074207288_value_13);
  // BDD node 72:vector_borrow(vector:(w64 1074190072), index:(w32 14), val_out:(w64 1074037872)[ -> (w64 1074204304)])
  // Module Ignore
  // BDD node 73:vector_return(vector:(w64 1074190072), index:(w32 14), value:(w64 1074204304)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_14(4);
  vector_table_1074190072_value_14.set(0, 4, 1);
  state->vector_table_1074190072.write(14, vector_table_1074190072_value_14);
  // BDD node 74:vector_borrow(vector:(w64 1074207288), index:(w32 14), val_out:(w64 1074037936)[ -> (w64 1074221520)])
  // Module Ignore
  // BDD node 75:vector_return(vector:(w64 1074207288), index:(w32 14), value:(w64 1074221520)[(w16 15)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_14(2);
  vector_table_1074207288_value_14.set(0, 2, 15);
  state->vector_table_1074207288.write(14, vector_table_1074207288_value_14);
  // BDD node 76:vector_borrow(vector:(w64 1074190072), index:(w32 15), val_out:(w64 1074037872)[ -> (w64 1074204328)])
  // Module Ignore
  // BDD node 77:vector_return(vector:(w64 1074190072), index:(w32 15), value:(w64 1074204328)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_15(4);
  vector_table_1074190072_value_15.set(0, 4, 0);
  state->vector_table_1074190072.write(15, vector_table_1074190072_value_15);
  // BDD node 78:vector_borrow(vector:(w64 1074207288), index:(w32 15), val_out:(w64 1074037936)[ -> (w64 1074221544)])
  // Module Ignore
  // BDD node 79:vector_return(vector:(w64 1074207288), index:(w32 15), value:(w64 1074221544)[(w16 14)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_15(2);
  vector_table_1074207288_value_15.set(0, 2, 14);
  state->vector_table_1074207288.write(15, vector_table_1074207288_value_15);
  // BDD node 80:vector_borrow(vector:(w64 1074190072), index:(w32 16), val_out:(w64 1074037872)[ -> (w64 1074204352)])
  // Module Ignore
  // BDD node 81:vector_return(vector:(w64 1074190072), index:(w32 16), value:(w64 1074204352)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_16(4);
  vector_table_1074190072_value_16.set(0, 4, 1);
  state->vector_table_1074190072.write(16, vector_table_1074190072_value_16);
  // BDD node 82:vector_borrow(vector:(w64 1074207288), index:(w32 16), val_out:(w64 1074037936)[ -> (w64 1074221568)])
  // Module Ignore
  // BDD node 83:vector_return(vector:(w64 1074207288), index:(w32 16), value:(w64 1074221568)[(w16 17)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_16(2);
  vector_table_1074207288_value_16.set(0, 2, 17);
  state->vector_table_1074207288.write(16, vector_table_1074207288_value_16);
  // BDD node 84:vector_borrow(vector:(w64 1074190072), index:(w32 17), val_out:(w64 1074037872)[ -> (w64 1074204376)])
  // Module Ignore
  // BDD node 85:vector_return(vector:(w64 1074190072), index:(w32 17), value:(w64 1074204376)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_17(4);
  vector_table_1074190072_value_17.set(0, 4, 0);
  state->vector_table_1074190072.write(17, vector_table_1074190072_value_17);
  // BDD node 86:vector_borrow(vector:(w64 1074207288), index:(w32 17), val_out:(w64 1074037936)[ -> (w64 1074221592)])
  // Module Ignore
  // BDD node 87:vector_return(vector:(w64 1074207288), index:(w32 17), value:(w64 1074221592)[(w16 16)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_17(2);
  vector_table_1074207288_value_17.set(0, 2, 16);
  state->vector_table_1074207288.write(17, vector_table_1074207288_value_17);
  // BDD node 88:vector_borrow(vector:(w64 1074190072), index:(w32 18), val_out:(w64 1074037872)[ -> (w64 1074204400)])
  // Module Ignore
  // BDD node 89:vector_return(vector:(w64 1074190072), index:(w32 18), value:(w64 1074204400)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_18(4);
  vector_table_1074190072_value_18.set(0, 4, 1);
  state->vector_table_1074190072.write(18, vector_table_1074190072_value_18);
  // BDD node 90:vector_borrow(vector:(w64 1074207288), index:(w32 18), val_out:(w64 1074037936)[ -> (w64 1074221616)])
  // Module Ignore
  // BDD node 91:vector_return(vector:(w64 1074207288), index:(w32 18), value:(w64 1074221616)[(w16 19)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_18(2);
  vector_table_1074207288_value_18.set(0, 2, 19);
  state->vector_table_1074207288.write(18, vector_table_1074207288_value_18);
  // BDD node 92:vector_borrow(vector:(w64 1074190072), index:(w32 19), val_out:(w64 1074037872)[ -> (w64 1074204424)])
  // Module Ignore
  // BDD node 93:vector_return(vector:(w64 1074190072), index:(w32 19), value:(w64 1074204424)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_19(4);
  vector_table_1074190072_value_19.set(0, 4, 0);
  state->vector_table_1074190072.write(19, vector_table_1074190072_value_19);
  // BDD node 94:vector_borrow(vector:(w64 1074207288), index:(w32 19), val_out:(w64 1074037936)[ -> (w64 1074221640)])
  // Module Ignore
  // BDD node 95:vector_return(vector:(w64 1074207288), index:(w32 19), value:(w64 1074221640)[(w16 18)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_19(2);
  vector_table_1074207288_value_19.set(0, 2, 18);
  state->vector_table_1074207288.write(19, vector_table_1074207288_value_19);
  // BDD node 96:vector_borrow(vector:(w64 1074190072), index:(w32 20), val_out:(w64 1074037872)[ -> (w64 1074204448)])
  // Module Ignore
  // BDD node 97:vector_return(vector:(w64 1074190072), index:(w32 20), value:(w64 1074204448)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_20(4);
  vector_table_1074190072_value_20.set(0, 4, 1);
  state->vector_table_1074190072.write(20, vector_table_1074190072_value_20);
  // BDD node 98:vector_borrow(vector:(w64 1074207288), index:(w32 20), val_out:(w64 1074037936)[ -> (w64 1074221664)])
  // Module Ignore
  // BDD node 99:vector_return(vector:(w64 1074207288), index:(w32 20), value:(w64 1074221664)[(w16 21)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_20(2);
  vector_table_1074207288_value_20.set(0, 2, 21);
  state->vector_table_1074207288.write(20, vector_table_1074207288_value_20);
  // BDD node 100:vector_borrow(vector:(w64 1074190072), index:(w32 21), val_out:(w64 1074037872)[ -> (w64 1074204472)])
  // Module Ignore
  // BDD node 101:vector_return(vector:(w64 1074190072), index:(w32 21), value:(w64 1074204472)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_21(4);
  vector_table_1074190072_value_21.set(0, 4, 0);
  state->vector_table_1074190072.write(21, vector_table_1074190072_value_21);
  // BDD node 102:vector_borrow(vector:(w64 1074207288), index:(w32 21), val_out:(w64 1074037936)[ -> (w64 1074221688)])
  // Module Ignore
  // BDD node 103:vector_return(vector:(w64 1074207288), index:(w32 21), value:(w64 1074221688)[(w16 20)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_21(2);
  vector_table_1074207288_value_21.set(0, 2, 20);
  state->vector_table_1074207288.write(21, vector_table_1074207288_value_21);
  // BDD node 104:vector_borrow(vector:(w64 1074190072), index:(w32 22), val_out:(w64 1074037872)[ -> (w64 1074204496)])
  // Module Ignore
  // BDD node 105:vector_return(vector:(w64 1074190072), index:(w32 22), value:(w64 1074204496)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_22(4);
  vector_table_1074190072_value_22.set(0, 4, 1);
  state->vector_table_1074190072.write(22, vector_table_1074190072_value_22);
  // BDD node 106:vector_borrow(vector:(w64 1074207288), index:(w32 22), val_out:(w64 1074037936)[ -> (w64 1074221712)])
  // Module Ignore
  // BDD node 107:vector_return(vector:(w64 1074207288), index:(w32 22), value:(w64 1074221712)[(w16 23)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_22(2);
  vector_table_1074207288_value_22.set(0, 2, 23);
  state->vector_table_1074207288.write(22, vector_table_1074207288_value_22);
  // BDD node 108:vector_borrow(vector:(w64 1074190072), index:(w32 23), val_out:(w64 1074037872)[ -> (w64 1074204520)])
  // Module Ignore
  // BDD node 109:vector_return(vector:(w64 1074190072), index:(w32 23), value:(w64 1074204520)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_23(4);
  vector_table_1074190072_value_23.set(0, 4, 0);
  state->vector_table_1074190072.write(23, vector_table_1074190072_value_23);
  // BDD node 110:vector_borrow(vector:(w64 1074207288), index:(w32 23), val_out:(w64 1074037936)[ -> (w64 1074221736)])
  // Module Ignore
  // BDD node 111:vector_return(vector:(w64 1074207288), index:(w32 23), value:(w64 1074221736)[(w16 22)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_23(2);
  vector_table_1074207288_value_23.set(0, 2, 22);
  state->vector_table_1074207288.write(23, vector_table_1074207288_value_23);
  // BDD node 112:vector_borrow(vector:(w64 1074190072), index:(w32 24), val_out:(w64 1074037872)[ -> (w64 1074204544)])
  // Module Ignore
  // BDD node 113:vector_return(vector:(w64 1074190072), index:(w32 24), value:(w64 1074204544)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_24(4);
  vector_table_1074190072_value_24.set(0, 4, 1);
  state->vector_table_1074190072.write(24, vector_table_1074190072_value_24);
  // BDD node 114:vector_borrow(vector:(w64 1074207288), index:(w32 24), val_out:(w64 1074037936)[ -> (w64 1074221760)])
  // Module Ignore
  // BDD node 115:vector_return(vector:(w64 1074207288), index:(w32 24), value:(w64 1074221760)[(w16 25)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_24(2);
  vector_table_1074207288_value_24.set(0, 2, 25);
  state->vector_table_1074207288.write(24, vector_table_1074207288_value_24);
  // BDD node 116:vector_borrow(vector:(w64 1074190072), index:(w32 25), val_out:(w64 1074037872)[ -> (w64 1074204568)])
  // Module Ignore
  // BDD node 117:vector_return(vector:(w64 1074190072), index:(w32 25), value:(w64 1074204568)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_25(4);
  vector_table_1074190072_value_25.set(0, 4, 0);
  state->vector_table_1074190072.write(25, vector_table_1074190072_value_25);
  // BDD node 118:vector_borrow(vector:(w64 1074207288), index:(w32 25), val_out:(w64 1074037936)[ -> (w64 1074221784)])
  // Module Ignore
  // BDD node 119:vector_return(vector:(w64 1074207288), index:(w32 25), value:(w64 1074221784)[(w16 24)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_25(2);
  vector_table_1074207288_value_25.set(0, 2, 24);
  state->vector_table_1074207288.write(25, vector_table_1074207288_value_25);
  // BDD node 120:vector_borrow(vector:(w64 1074190072), index:(w32 26), val_out:(w64 1074037872)[ -> (w64 1074204592)])
  // Module Ignore
  // BDD node 121:vector_return(vector:(w64 1074190072), index:(w32 26), value:(w64 1074204592)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_26(4);
  vector_table_1074190072_value_26.set(0, 4, 1);
  state->vector_table_1074190072.write(26, vector_table_1074190072_value_26);
  // BDD node 122:vector_borrow(vector:(w64 1074207288), index:(w32 26), val_out:(w64 1074037936)[ -> (w64 1074221808)])
  // Module Ignore
  // BDD node 123:vector_return(vector:(w64 1074207288), index:(w32 26), value:(w64 1074221808)[(w16 27)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_26(2);
  vector_table_1074207288_value_26.set(0, 2, 27);
  state->vector_table_1074207288.write(26, vector_table_1074207288_value_26);
  // BDD node 124:vector_borrow(vector:(w64 1074190072), index:(w32 27), val_out:(w64 1074037872)[ -> (w64 1074204616)])
  // Module Ignore
  // BDD node 125:vector_return(vector:(w64 1074190072), index:(w32 27), value:(w64 1074204616)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_27(4);
  vector_table_1074190072_value_27.set(0, 4, 0);
  state->vector_table_1074190072.write(27, vector_table_1074190072_value_27);
  // BDD node 126:vector_borrow(vector:(w64 1074207288), index:(w32 27), val_out:(w64 1074037936)[ -> (w64 1074221832)])
  // Module Ignore
  // BDD node 127:vector_return(vector:(w64 1074207288), index:(w32 27), value:(w64 1074221832)[(w16 26)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_27(2);
  vector_table_1074207288_value_27.set(0, 2, 26);
  state->vector_table_1074207288.write(27, vector_table_1074207288_value_27);
  // BDD node 128:vector_borrow(vector:(w64 1074190072), index:(w32 28), val_out:(w64 1074037872)[ -> (w64 1074204640)])
  // Module Ignore
  // BDD node 129:vector_return(vector:(w64 1074190072), index:(w32 28), value:(w64 1074204640)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_28(4);
  vector_table_1074190072_value_28.set(0, 4, 1);
  state->vector_table_1074190072.write(28, vector_table_1074190072_value_28);
  // BDD node 130:vector_borrow(vector:(w64 1074207288), index:(w32 28), val_out:(w64 1074037936)[ -> (w64 1074221856)])
  // Module Ignore
  // BDD node 131:vector_return(vector:(w64 1074207288), index:(w32 28), value:(w64 1074221856)[(w16 29)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_28(2);
  vector_table_1074207288_value_28.set(0, 2, 29);
  state->vector_table_1074207288.write(28, vector_table_1074207288_value_28);
  // BDD node 132:vector_borrow(vector:(w64 1074190072), index:(w32 29), val_out:(w64 1074037872)[ -> (w64 1074204664)])
  // Module Ignore
  // BDD node 133:vector_return(vector:(w64 1074190072), index:(w32 29), value:(w64 1074204664)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_29(4);
  vector_table_1074190072_value_29.set(0, 4, 0);
  state->vector_table_1074190072.write(29, vector_table_1074190072_value_29);
  // BDD node 134:vector_borrow(vector:(w64 1074207288), index:(w32 29), val_out:(w64 1074037936)[ -> (w64 1074221880)])
  // Module Ignore
  // BDD node 135:vector_return(vector:(w64 1074207288), index:(w32 29), value:(w64 1074221880)[(w16 28)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_29(2);
  vector_table_1074207288_value_29.set(0, 2, 28);
  state->vector_table_1074207288.write(29, vector_table_1074207288_value_29);
  // BDD node 136:vector_borrow(vector:(w64 1074190072), index:(w32 30), val_out:(w64 1074037872)[ -> (w64 1074204688)])
  // Module Ignore
  // BDD node 137:vector_return(vector:(w64 1074190072), index:(w32 30), value:(w64 1074204688)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_30(4);
  vector_table_1074190072_value_30.set(0, 4, 1);
  state->vector_table_1074190072.write(30, vector_table_1074190072_value_30);
  // BDD node 138:vector_borrow(vector:(w64 1074207288), index:(w32 30), val_out:(w64 1074037936)[ -> (w64 1074221904)])
  // Module Ignore
  // BDD node 139:vector_return(vector:(w64 1074207288), index:(w32 30), value:(w64 1074221904)[(w16 31)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_30(2);
  vector_table_1074207288_value_30.set(0, 2, 31);
  state->vector_table_1074207288.write(30, vector_table_1074207288_value_30);
  // BDD node 140:vector_borrow(vector:(w64 1074190072), index:(w32 31), val_out:(w64 1074037872)[ -> (w64 1074204712)])
  // Module Ignore
  // BDD node 141:vector_return(vector:(w64 1074190072), index:(w32 31), value:(w64 1074204712)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074190072_value_31(4);
  vector_table_1074190072_value_31.set(0, 4, 0);
  state->vector_table_1074190072.write(31, vector_table_1074190072_value_31);
  // BDD node 142:vector_borrow(vector:(w64 1074207288), index:(w32 31), val_out:(w64 1074037936)[ -> (w64 1074221928)])
  // Module Ignore
  // BDD node 143:vector_return(vector:(w64 1074207288), index:(w32 31), value:(w64 1074221928)[(w16 30)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1074207288_value_31(2);
  vector_table_1074207288_value_31.set(0, 2, 30);
  state->vector_table_1074207288.write(31, vector_table_1074207288_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {

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

  // The packet as the NF sees it: without the cpu headers the data plane put in front of it.
  const u16 pkt_len = static_cast<u16>(size - packet_consumed);

  cpu_hdr->egress_dev = 0;
  cpu_hdr->trigger_dataplane_execution = 0;





  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
