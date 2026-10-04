#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  VectorRegister vector_register_1249835474944;
  libnf::Vector *cpu_vector_1247687987200 = nullptr;
  libnf::Vector *cpu_vector_1251982962688 = nullptr;
  libnf::Vector *cpu_vector_1248761733120 = nullptr;
  libnf::Vector *cpu_vector_1246614245376 = nullptr;
  libnf::Vector *cpu_vector_1253056708608 = nullptr;
  libnf::Vector *cpu_vector_1250909220864 = nullptr;
  libnf::Vector *cpu_vector_1248224862208 = nullptr;
  CRC32 crc32_hasher_1245003644928;
  CRC32 crc32_hasher_1244735209472;
  CRC32 crc32_hasher_1245272080384;
  CRC32 crc32_hasher_1244600991744;
  CRC32 crc32_hasher_1245406298112;
  CRC32 crc32_hasher_1244869427200;
  VectorTable vector_table_1246077374464;
  VectorRegister vector_register_1252519837696;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      vector_register_1249835474944("vector_register_1249835474944",{"Ingress.vector_register_1249835474944_0",}, 1000LL),
      crc32_hasher_1245003644928(libnf::crc32_config{"", 0x04c11db7, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1244735209472(libnf::crc32_config{"", 0x7b17a39f, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1245272080384(libnf::crc32_config{"", 0x99f29aad, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1244600991744(libnf::crc32_config{"", 0x21bca2c3, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1245406298112(libnf::crc32_config{"", 0x0c596849, true, 0xffffffff, 0xffffffff}),
      crc32_hasher_1244869427200(libnf::crc32_config{"", 0x3553d717, true, 0xffffffff, 0xffffffff}),
      vector_table_1246077374464("vector_table_1246077374464",{"Ingress.vector_table_1246077374464_157",}),
      vector_register_1252519837696("vector_register_1252519837696",{"Ingress.vector_register_1252519837696_0",})
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
  // BDD node 0:vector_allocate(elem_size:(w32 4), capacity:(w32 256), vector_out:(w64 1243594358784)[(w64 0) -> (w64 1249835474944)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 4), capacity:(w32 65536), vector_out:(w64 1243594358792)[(w64 0) -> (w64 1247687987200)])
  // Module VectorAllocate
  libnf::vector_allocate(4, 65536, &state->cpu_vector_1247687987200);
  // BDD node 2:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1242453508096)[(w64 0) -> (w64 1251982962688)])
  // Module VectorAllocate
  libnf::vector_allocate(8, 1024, &state->cpu_vector_1251982962688);
  // BDD node 3:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1242453508104)[(w64 0) -> (w64 1248761733120)])
  // Module VectorAllocate
  libnf::vector_allocate(8, 1024, &state->cpu_vector_1248761733120);
  // BDD node 4:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1242453508112)[(w64 0) -> (w64 1246614245376)])
  // Module VectorAllocate
  libnf::vector_allocate(8, 1024, &state->cpu_vector_1246614245376);
  // BDD node 5:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1242453508120)[(w64 0) -> (w64 1253056708608)])
  // Module VectorAllocate
  libnf::vector_allocate(8, 1024, &state->cpu_vector_1253056708608);
  // BDD node 6:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1242453508128)[(w64 0) -> (w64 1250909220864)])
  // Module VectorAllocate
  libnf::vector_allocate(8, 1024, &state->cpu_vector_1250909220864);
  // BDD node 7:vector_allocate(elem_size:(w32 8), capacity:(w32 1024), vector_out:(w64 1242453508136)[(w64 0) -> (w64 1248224862208)])
  // Module VectorAllocate
  libnf::vector_allocate(8, 1024, &state->cpu_vector_1248224862208);
  // BDD node 8:crc32_hasher_init(hasher:(w64 1245003644928), polynomial:(w32 79764919), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 9:crc32_hasher_init(hasher:(w64 1244735209472), polynomial:(w32 2065146783), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 10:crc32_hasher_init(hasher:(w64 1245272080384), polynomial:(w32 2582813357), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 11:crc32_hasher_init(hasher:(w64 1244600991744), polynomial:(w32 566010563), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 12:crc32_hasher_init(hasher:(w64 1245406298112), polynomial:(w32 207185993), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 13:crc32_hasher_init(hasher:(w64 1244869427200), polynomial:(w32 894686999), reversed:(w32 1), init:(w32 4294967295), xor_out:(w32 4294967295))
  // Module Crc32HasherInit
  // BDD node 14:vector_allocate(elem_size:(w32 4), capacity:(w32 32), vector_out:(w64 1243594358816)[(w64 0) -> (w64 1246077374464)])
  // Module DataplaneVectorTableAllocate
  // BDD node 15:vector_allocate(elem_size:(w32 2), capacity:(w32 32), vector_out:(w64 1243594358824)[(w64 0) -> (w64 1252519837696)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 16:vector_borrow(vector:(w64 1246077374464), index:(w32 0), val_out:(w64 1649812701184)[ -> (w64 1240167088128)])
  // Module Ignore
  // BDD node 17:vector_return(vector:(w64 1246077374464), index:(w32 0), value:(w64 1240167088128)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_0(4);
  vector_table_1246077374464_value_0.set(0, 4, 1);
  state->vector_table_1246077374464.write(0, vector_table_1246077374464_value_0);
  // BDD node 18:vector_borrow(vector:(w64 1252519837696), index:(w32 0), val_out:(w64 1649829478400)[ -> (w64 1240099979264)])
  // Module Ignore
  // BDD node 19:vector_return(vector:(w64 1252519837696), index:(w32 0), value:(w64 1240099979264)[(w16 1)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_0(2);
  vector_register_1252519837696_value_0.set(0, 2, 1);
  state->vector_register_1252519837696.put(0, vector_register_1252519837696_value_0);
  // BDD node 20:vector_borrow(vector:(w64 1246077374464), index:(w32 1), val_out:(w64 1649812701184)[ -> (w64 1239103832064)])
  // Module Ignore
  // BDD node 21:vector_return(vector:(w64 1246077374464), index:(w32 1), value:(w64 1239103832064)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_1(4);
  vector_table_1246077374464_value_1.set(0, 4, 0);
  state->vector_table_1246077374464.write(1, vector_table_1246077374464_value_1);
  // BDD node 22:vector_borrow(vector:(w64 1252519837696), index:(w32 1), val_out:(w64 1649829478400)[ -> (w64 1239170940928)])
  // Module Ignore
  // BDD node 23:vector_return(vector:(w64 1252519837696), index:(w32 1), value:(w64 1239170940928)[(w16 0)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_1(2);
  vector_register_1252519837696_value_1.set(0, 2, 0);
  state->vector_register_1252519837696.put(1, vector_register_1252519837696_value_1);
  // BDD node 24:vector_borrow(vector:(w64 1246077374464), index:(w32 2), val_out:(w64 1649812701184)[ -> (w64 1240166039552)])
  // Module Ignore
  // BDD node 25:vector_return(vector:(w64 1246077374464), index:(w32 2), value:(w64 1240166039552)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_2(4);
  vector_table_1246077374464_value_2.set(0, 4, 1);
  state->vector_table_1246077374464.write(2, vector_table_1246077374464_value_2);
  // BDD node 26:vector_borrow(vector:(w64 1252519837696), index:(w32 2), val_out:(w64 1649829478400)[ -> (w64 1240098930688)])
  // Module Ignore
  // BDD node 27:vector_return(vector:(w64 1252519837696), index:(w32 2), value:(w64 1240098930688)[(w16 3)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_2(2);
  vector_register_1252519837696_value_2.set(0, 2, 3);
  state->vector_register_1252519837696.put(2, vector_register_1252519837696_value_2);
  // BDD node 28:vector_borrow(vector:(w64 1246077374464), index:(w32 3), val_out:(w64 1649812701184)[ -> (w64 1239104880640)])
  // Module Ignore
  // BDD node 29:vector_return(vector:(w64 1246077374464), index:(w32 3), value:(w64 1239104880640)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_3(4);
  vector_table_1246077374464_value_3.set(0, 4, 0);
  state->vector_table_1246077374464.write(3, vector_table_1246077374464_value_3);
  // BDD node 30:vector_borrow(vector:(w64 1252519837696), index:(w32 3), val_out:(w64 1649829478400)[ -> (w64 1239171989504)])
  // Module Ignore
  // BDD node 31:vector_return(vector:(w64 1252519837696), index:(w32 3), value:(w64 1239171989504)[(w16 2)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_3(2);
  vector_register_1252519837696_value_3.set(0, 2, 2);
  state->vector_register_1252519837696.put(3, vector_register_1252519837696_value_3);
  // BDD node 32:vector_borrow(vector:(w64 1246077374464), index:(w32 4), val_out:(w64 1649812701184)[ -> (w64 1240164990976)])
  // Module Ignore
  // BDD node 33:vector_return(vector:(w64 1246077374464), index:(w32 4), value:(w64 1240164990976)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_4(4);
  vector_table_1246077374464_value_4.set(0, 4, 1);
  state->vector_table_1246077374464.write(4, vector_table_1246077374464_value_4);
  // BDD node 34:vector_borrow(vector:(w64 1252519837696), index:(w32 4), val_out:(w64 1649829478400)[ -> (w64 1240097882112)])
  // Module Ignore
  // BDD node 35:vector_return(vector:(w64 1252519837696), index:(w32 4), value:(w64 1240097882112)[(w16 5)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_4(2);
  vector_register_1252519837696_value_4.set(0, 2, 5);
  state->vector_register_1252519837696.put(4, vector_register_1252519837696_value_4);
  // BDD node 36:vector_borrow(vector:(w64 1246077374464), index:(w32 5), val_out:(w64 1649812701184)[ -> (w64 1239105929216)])
  // Module Ignore
  // BDD node 37:vector_return(vector:(w64 1246077374464), index:(w32 5), value:(w64 1239105929216)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_5(4);
  vector_table_1246077374464_value_5.set(0, 4, 0);
  state->vector_table_1246077374464.write(5, vector_table_1246077374464_value_5);
  // BDD node 38:vector_borrow(vector:(w64 1252519837696), index:(w32 5), val_out:(w64 1649829478400)[ -> (w64 1239173038080)])
  // Module Ignore
  // BDD node 39:vector_return(vector:(w64 1252519837696), index:(w32 5), value:(w64 1239173038080)[(w16 4)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_5(2);
  vector_register_1252519837696_value_5.set(0, 2, 4);
  state->vector_register_1252519837696.put(5, vector_register_1252519837696_value_5);
  // BDD node 40:vector_borrow(vector:(w64 1246077374464), index:(w32 6), val_out:(w64 1649812701184)[ -> (w64 1240163942400)])
  // Module Ignore
  // BDD node 41:vector_return(vector:(w64 1246077374464), index:(w32 6), value:(w64 1240163942400)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_6(4);
  vector_table_1246077374464_value_6.set(0, 4, 1);
  state->vector_table_1246077374464.write(6, vector_table_1246077374464_value_6);
  // BDD node 42:vector_borrow(vector:(w64 1252519837696), index:(w32 6), val_out:(w64 1649829478400)[ -> (w64 1240096833536)])
  // Module Ignore
  // BDD node 43:vector_return(vector:(w64 1252519837696), index:(w32 6), value:(w64 1240096833536)[(w16 7)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_6(2);
  vector_register_1252519837696_value_6.set(0, 2, 7);
  state->vector_register_1252519837696.put(6, vector_register_1252519837696_value_6);
  // BDD node 44:vector_borrow(vector:(w64 1246077374464), index:(w32 7), val_out:(w64 1649812701184)[ -> (w64 1239106977792)])
  // Module Ignore
  // BDD node 45:vector_return(vector:(w64 1246077374464), index:(w32 7), value:(w64 1239106977792)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_7(4);
  vector_table_1246077374464_value_7.set(0, 4, 0);
  state->vector_table_1246077374464.write(7, vector_table_1246077374464_value_7);
  // BDD node 46:vector_borrow(vector:(w64 1252519837696), index:(w32 7), val_out:(w64 1649829478400)[ -> (w64 1239174086656)])
  // Module Ignore
  // BDD node 47:vector_return(vector:(w64 1252519837696), index:(w32 7), value:(w64 1239174086656)[(w16 6)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_7(2);
  vector_register_1252519837696_value_7.set(0, 2, 6);
  state->vector_register_1252519837696.put(7, vector_register_1252519837696_value_7);
  // BDD node 48:vector_borrow(vector:(w64 1246077374464), index:(w32 8), val_out:(w64 1649812701184)[ -> (w64 1240162893824)])
  // Module Ignore
  // BDD node 49:vector_return(vector:(w64 1246077374464), index:(w32 8), value:(w64 1240162893824)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_8(4);
  vector_table_1246077374464_value_8.set(0, 4, 1);
  state->vector_table_1246077374464.write(8, vector_table_1246077374464_value_8);
  // BDD node 50:vector_borrow(vector:(w64 1252519837696), index:(w32 8), val_out:(w64 1649829478400)[ -> (w64 1240095784960)])
  // Module Ignore
  // BDD node 51:vector_return(vector:(w64 1252519837696), index:(w32 8), value:(w64 1240095784960)[(w16 9)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_8(2);
  vector_register_1252519837696_value_8.set(0, 2, 9);
  state->vector_register_1252519837696.put(8, vector_register_1252519837696_value_8);
  // BDD node 52:vector_borrow(vector:(w64 1246077374464), index:(w32 9), val_out:(w64 1649812701184)[ -> (w64 1239108026368)])
  // Module Ignore
  // BDD node 53:vector_return(vector:(w64 1246077374464), index:(w32 9), value:(w64 1239108026368)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_9(4);
  vector_table_1246077374464_value_9.set(0, 4, 0);
  state->vector_table_1246077374464.write(9, vector_table_1246077374464_value_9);
  // BDD node 54:vector_borrow(vector:(w64 1252519837696), index:(w32 9), val_out:(w64 1649829478400)[ -> (w64 1239175135232)])
  // Module Ignore
  // BDD node 55:vector_return(vector:(w64 1252519837696), index:(w32 9), value:(w64 1239175135232)[(w16 8)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_9(2);
  vector_register_1252519837696_value_9.set(0, 2, 8);
  state->vector_register_1252519837696.put(9, vector_register_1252519837696_value_9);
  // BDD node 56:vector_borrow(vector:(w64 1246077374464), index:(w32 10), val_out:(w64 1649812701184)[ -> (w64 1240161845248)])
  // Module Ignore
  // BDD node 57:vector_return(vector:(w64 1246077374464), index:(w32 10), value:(w64 1240161845248)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_10(4);
  vector_table_1246077374464_value_10.set(0, 4, 1);
  state->vector_table_1246077374464.write(10, vector_table_1246077374464_value_10);
  // BDD node 58:vector_borrow(vector:(w64 1252519837696), index:(w32 10), val_out:(w64 1649829478400)[ -> (w64 1240094736384)])
  // Module Ignore
  // BDD node 59:vector_return(vector:(w64 1252519837696), index:(w32 10), value:(w64 1240094736384)[(w16 11)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_10(2);
  vector_register_1252519837696_value_10.set(0, 2, 11);
  state->vector_register_1252519837696.put(10, vector_register_1252519837696_value_10);
  // BDD node 60:vector_borrow(vector:(w64 1246077374464), index:(w32 11), val_out:(w64 1649812701184)[ -> (w64 1239109074944)])
  // Module Ignore
  // BDD node 61:vector_return(vector:(w64 1246077374464), index:(w32 11), value:(w64 1239109074944)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_11(4);
  vector_table_1246077374464_value_11.set(0, 4, 0);
  state->vector_table_1246077374464.write(11, vector_table_1246077374464_value_11);
  // BDD node 62:vector_borrow(vector:(w64 1252519837696), index:(w32 11), val_out:(w64 1649829478400)[ -> (w64 1239176183808)])
  // Module Ignore
  // BDD node 63:vector_return(vector:(w64 1252519837696), index:(w32 11), value:(w64 1239176183808)[(w16 10)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_11(2);
  vector_register_1252519837696_value_11.set(0, 2, 10);
  state->vector_register_1252519837696.put(11, vector_register_1252519837696_value_11);
  // BDD node 64:vector_borrow(vector:(w64 1246077374464), index:(w32 12), val_out:(w64 1649812701184)[ -> (w64 1240160796672)])
  // Module Ignore
  // BDD node 65:vector_return(vector:(w64 1246077374464), index:(w32 12), value:(w64 1240160796672)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_12(4);
  vector_table_1246077374464_value_12.set(0, 4, 1);
  state->vector_table_1246077374464.write(12, vector_table_1246077374464_value_12);
  // BDD node 66:vector_borrow(vector:(w64 1252519837696), index:(w32 12), val_out:(w64 1649829478400)[ -> (w64 1240093687808)])
  // Module Ignore
  // BDD node 67:vector_return(vector:(w64 1252519837696), index:(w32 12), value:(w64 1240093687808)[(w16 13)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_12(2);
  vector_register_1252519837696_value_12.set(0, 2, 13);
  state->vector_register_1252519837696.put(12, vector_register_1252519837696_value_12);
  // BDD node 68:vector_borrow(vector:(w64 1246077374464), index:(w32 13), val_out:(w64 1649812701184)[ -> (w64 1239110123520)])
  // Module Ignore
  // BDD node 69:vector_return(vector:(w64 1246077374464), index:(w32 13), value:(w64 1239110123520)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_13(4);
  vector_table_1246077374464_value_13.set(0, 4, 0);
  state->vector_table_1246077374464.write(13, vector_table_1246077374464_value_13);
  // BDD node 70:vector_borrow(vector:(w64 1252519837696), index:(w32 13), val_out:(w64 1649829478400)[ -> (w64 1239177232384)])
  // Module Ignore
  // BDD node 71:vector_return(vector:(w64 1252519837696), index:(w32 13), value:(w64 1239177232384)[(w16 12)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_13(2);
  vector_register_1252519837696_value_13.set(0, 2, 12);
  state->vector_register_1252519837696.put(13, vector_register_1252519837696_value_13);
  // BDD node 72:vector_borrow(vector:(w64 1246077374464), index:(w32 14), val_out:(w64 1649812701184)[ -> (w64 1240159748096)])
  // Module Ignore
  // BDD node 73:vector_return(vector:(w64 1246077374464), index:(w32 14), value:(w64 1240159748096)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_14(4);
  vector_table_1246077374464_value_14.set(0, 4, 1);
  state->vector_table_1246077374464.write(14, vector_table_1246077374464_value_14);
  // BDD node 74:vector_borrow(vector:(w64 1252519837696), index:(w32 14), val_out:(w64 1649829478400)[ -> (w64 1240092639232)])
  // Module Ignore
  // BDD node 75:vector_return(vector:(w64 1252519837696), index:(w32 14), value:(w64 1240092639232)[(w16 15)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_14(2);
  vector_register_1252519837696_value_14.set(0, 2, 15);
  state->vector_register_1252519837696.put(14, vector_register_1252519837696_value_14);
  // BDD node 76:vector_borrow(vector:(w64 1246077374464), index:(w32 15), val_out:(w64 1649812701184)[ -> (w64 1239111172096)])
  // Module Ignore
  // BDD node 77:vector_return(vector:(w64 1246077374464), index:(w32 15), value:(w64 1239111172096)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_15(4);
  vector_table_1246077374464_value_15.set(0, 4, 0);
  state->vector_table_1246077374464.write(15, vector_table_1246077374464_value_15);
  // BDD node 78:vector_borrow(vector:(w64 1252519837696), index:(w32 15), val_out:(w64 1649829478400)[ -> (w64 1239178280960)])
  // Module Ignore
  // BDD node 79:vector_return(vector:(w64 1252519837696), index:(w32 15), value:(w64 1239178280960)[(w16 14)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_15(2);
  vector_register_1252519837696_value_15.set(0, 2, 14);
  state->vector_register_1252519837696.put(15, vector_register_1252519837696_value_15);
  // BDD node 80:vector_borrow(vector:(w64 1246077374464), index:(w32 16), val_out:(w64 1649812701184)[ -> (w64 1240158699520)])
  // Module Ignore
  // BDD node 81:vector_return(vector:(w64 1246077374464), index:(w32 16), value:(w64 1240158699520)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_16(4);
  vector_table_1246077374464_value_16.set(0, 4, 1);
  state->vector_table_1246077374464.write(16, vector_table_1246077374464_value_16);
  // BDD node 82:vector_borrow(vector:(w64 1252519837696), index:(w32 16), val_out:(w64 1649829478400)[ -> (w64 1240091590656)])
  // Module Ignore
  // BDD node 83:vector_return(vector:(w64 1252519837696), index:(w32 16), value:(w64 1240091590656)[(w16 17)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_16(2);
  vector_register_1252519837696_value_16.set(0, 2, 17);
  state->vector_register_1252519837696.put(16, vector_register_1252519837696_value_16);
  // BDD node 84:vector_borrow(vector:(w64 1246077374464), index:(w32 17), val_out:(w64 1649812701184)[ -> (w64 1239112220672)])
  // Module Ignore
  // BDD node 85:vector_return(vector:(w64 1246077374464), index:(w32 17), value:(w64 1239112220672)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_17(4);
  vector_table_1246077374464_value_17.set(0, 4, 0);
  state->vector_table_1246077374464.write(17, vector_table_1246077374464_value_17);
  // BDD node 86:vector_borrow(vector:(w64 1252519837696), index:(w32 17), val_out:(w64 1649829478400)[ -> (w64 1239179329536)])
  // Module Ignore
  // BDD node 87:vector_return(vector:(w64 1252519837696), index:(w32 17), value:(w64 1239179329536)[(w16 16)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_17(2);
  vector_register_1252519837696_value_17.set(0, 2, 16);
  state->vector_register_1252519837696.put(17, vector_register_1252519837696_value_17);
  // BDD node 88:vector_borrow(vector:(w64 1246077374464), index:(w32 18), val_out:(w64 1649812701184)[ -> (w64 1240157650944)])
  // Module Ignore
  // BDD node 89:vector_return(vector:(w64 1246077374464), index:(w32 18), value:(w64 1240157650944)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_18(4);
  vector_table_1246077374464_value_18.set(0, 4, 1);
  state->vector_table_1246077374464.write(18, vector_table_1246077374464_value_18);
  // BDD node 90:vector_borrow(vector:(w64 1252519837696), index:(w32 18), val_out:(w64 1649829478400)[ -> (w64 1240090542080)])
  // Module Ignore
  // BDD node 91:vector_return(vector:(w64 1252519837696), index:(w32 18), value:(w64 1240090542080)[(w16 19)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_18(2);
  vector_register_1252519837696_value_18.set(0, 2, 19);
  state->vector_register_1252519837696.put(18, vector_register_1252519837696_value_18);
  // BDD node 92:vector_borrow(vector:(w64 1246077374464), index:(w32 19), val_out:(w64 1649812701184)[ -> (w64 1239113269248)])
  // Module Ignore
  // BDD node 93:vector_return(vector:(w64 1246077374464), index:(w32 19), value:(w64 1239113269248)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_19(4);
  vector_table_1246077374464_value_19.set(0, 4, 0);
  state->vector_table_1246077374464.write(19, vector_table_1246077374464_value_19);
  // BDD node 94:vector_borrow(vector:(w64 1252519837696), index:(w32 19), val_out:(w64 1649829478400)[ -> (w64 1239180378112)])
  // Module Ignore
  // BDD node 95:vector_return(vector:(w64 1252519837696), index:(w32 19), value:(w64 1239180378112)[(w16 18)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_19(2);
  vector_register_1252519837696_value_19.set(0, 2, 18);
  state->vector_register_1252519837696.put(19, vector_register_1252519837696_value_19);
  // BDD node 96:vector_borrow(vector:(w64 1246077374464), index:(w32 20), val_out:(w64 1649812701184)[ -> (w64 1240156602368)])
  // Module Ignore
  // BDD node 97:vector_return(vector:(w64 1246077374464), index:(w32 20), value:(w64 1240156602368)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_20(4);
  vector_table_1246077374464_value_20.set(0, 4, 1);
  state->vector_table_1246077374464.write(20, vector_table_1246077374464_value_20);
  // BDD node 98:vector_borrow(vector:(w64 1252519837696), index:(w32 20), val_out:(w64 1649829478400)[ -> (w64 1240089493504)])
  // Module Ignore
  // BDD node 99:vector_return(vector:(w64 1252519837696), index:(w32 20), value:(w64 1240089493504)[(w16 21)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_20(2);
  vector_register_1252519837696_value_20.set(0, 2, 21);
  state->vector_register_1252519837696.put(20, vector_register_1252519837696_value_20);
  // BDD node 100:vector_borrow(vector:(w64 1246077374464), index:(w32 21), val_out:(w64 1649812701184)[ -> (w64 1239114317824)])
  // Module Ignore
  // BDD node 101:vector_return(vector:(w64 1246077374464), index:(w32 21), value:(w64 1239114317824)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_21(4);
  vector_table_1246077374464_value_21.set(0, 4, 0);
  state->vector_table_1246077374464.write(21, vector_table_1246077374464_value_21);
  // BDD node 102:vector_borrow(vector:(w64 1252519837696), index:(w32 21), val_out:(w64 1649829478400)[ -> (w64 1239181426688)])
  // Module Ignore
  // BDD node 103:vector_return(vector:(w64 1252519837696), index:(w32 21), value:(w64 1239181426688)[(w16 20)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_21(2);
  vector_register_1252519837696_value_21.set(0, 2, 20);
  state->vector_register_1252519837696.put(21, vector_register_1252519837696_value_21);
  // BDD node 104:vector_borrow(vector:(w64 1246077374464), index:(w32 22), val_out:(w64 1649812701184)[ -> (w64 1240155553792)])
  // Module Ignore
  // BDD node 105:vector_return(vector:(w64 1246077374464), index:(w32 22), value:(w64 1240155553792)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_22(4);
  vector_table_1246077374464_value_22.set(0, 4, 1);
  state->vector_table_1246077374464.write(22, vector_table_1246077374464_value_22);
  // BDD node 106:vector_borrow(vector:(w64 1252519837696), index:(w32 22), val_out:(w64 1649829478400)[ -> (w64 1240088444928)])
  // Module Ignore
  // BDD node 107:vector_return(vector:(w64 1252519837696), index:(w32 22), value:(w64 1240088444928)[(w16 23)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_22(2);
  vector_register_1252519837696_value_22.set(0, 2, 23);
  state->vector_register_1252519837696.put(22, vector_register_1252519837696_value_22);
  // BDD node 108:vector_borrow(vector:(w64 1246077374464), index:(w32 23), val_out:(w64 1649812701184)[ -> (w64 1239115366400)])
  // Module Ignore
  // BDD node 109:vector_return(vector:(w64 1246077374464), index:(w32 23), value:(w64 1239115366400)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_23(4);
  vector_table_1246077374464_value_23.set(0, 4, 0);
  state->vector_table_1246077374464.write(23, vector_table_1246077374464_value_23);
  // BDD node 110:vector_borrow(vector:(w64 1252519837696), index:(w32 23), val_out:(w64 1649829478400)[ -> (w64 1239182475264)])
  // Module Ignore
  // BDD node 111:vector_return(vector:(w64 1252519837696), index:(w32 23), value:(w64 1239182475264)[(w16 22)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_23(2);
  vector_register_1252519837696_value_23.set(0, 2, 22);
  state->vector_register_1252519837696.put(23, vector_register_1252519837696_value_23);
  // BDD node 112:vector_borrow(vector:(w64 1246077374464), index:(w32 24), val_out:(w64 1649812701184)[ -> (w64 1240154505216)])
  // Module Ignore
  // BDD node 113:vector_return(vector:(w64 1246077374464), index:(w32 24), value:(w64 1240154505216)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_24(4);
  vector_table_1246077374464_value_24.set(0, 4, 1);
  state->vector_table_1246077374464.write(24, vector_table_1246077374464_value_24);
  // BDD node 114:vector_borrow(vector:(w64 1252519837696), index:(w32 24), val_out:(w64 1649829478400)[ -> (w64 1240087396352)])
  // Module Ignore
  // BDD node 115:vector_return(vector:(w64 1252519837696), index:(w32 24), value:(w64 1240087396352)[(w16 25)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_24(2);
  vector_register_1252519837696_value_24.set(0, 2, 25);
  state->vector_register_1252519837696.put(24, vector_register_1252519837696_value_24);
  // BDD node 116:vector_borrow(vector:(w64 1246077374464), index:(w32 25), val_out:(w64 1649812701184)[ -> (w64 1239116414976)])
  // Module Ignore
  // BDD node 117:vector_return(vector:(w64 1246077374464), index:(w32 25), value:(w64 1239116414976)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_25(4);
  vector_table_1246077374464_value_25.set(0, 4, 0);
  state->vector_table_1246077374464.write(25, vector_table_1246077374464_value_25);
  // BDD node 118:vector_borrow(vector:(w64 1252519837696), index:(w32 25), val_out:(w64 1649829478400)[ -> (w64 1239183523840)])
  // Module Ignore
  // BDD node 119:vector_return(vector:(w64 1252519837696), index:(w32 25), value:(w64 1239183523840)[(w16 24)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_25(2);
  vector_register_1252519837696_value_25.set(0, 2, 24);
  state->vector_register_1252519837696.put(25, vector_register_1252519837696_value_25);
  // BDD node 120:vector_borrow(vector:(w64 1246077374464), index:(w32 26), val_out:(w64 1649812701184)[ -> (w64 1240153456640)])
  // Module Ignore
  // BDD node 121:vector_return(vector:(w64 1246077374464), index:(w32 26), value:(w64 1240153456640)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_26(4);
  vector_table_1246077374464_value_26.set(0, 4, 1);
  state->vector_table_1246077374464.write(26, vector_table_1246077374464_value_26);
  // BDD node 122:vector_borrow(vector:(w64 1252519837696), index:(w32 26), val_out:(w64 1649829478400)[ -> (w64 1240086347776)])
  // Module Ignore
  // BDD node 123:vector_return(vector:(w64 1252519837696), index:(w32 26), value:(w64 1240086347776)[(w16 27)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_26(2);
  vector_register_1252519837696_value_26.set(0, 2, 27);
  state->vector_register_1252519837696.put(26, vector_register_1252519837696_value_26);
  // BDD node 124:vector_borrow(vector:(w64 1246077374464), index:(w32 27), val_out:(w64 1649812701184)[ -> (w64 1239117463552)])
  // Module Ignore
  // BDD node 125:vector_return(vector:(w64 1246077374464), index:(w32 27), value:(w64 1239117463552)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_27(4);
  vector_table_1246077374464_value_27.set(0, 4, 0);
  state->vector_table_1246077374464.write(27, vector_table_1246077374464_value_27);
  // BDD node 126:vector_borrow(vector:(w64 1252519837696), index:(w32 27), val_out:(w64 1649829478400)[ -> (w64 1239184572416)])
  // Module Ignore
  // BDD node 127:vector_return(vector:(w64 1252519837696), index:(w32 27), value:(w64 1239184572416)[(w16 26)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_27(2);
  vector_register_1252519837696_value_27.set(0, 2, 26);
  state->vector_register_1252519837696.put(27, vector_register_1252519837696_value_27);
  // BDD node 128:vector_borrow(vector:(w64 1246077374464), index:(w32 28), val_out:(w64 1649812701184)[ -> (w64 1240152408064)])
  // Module Ignore
  // BDD node 129:vector_return(vector:(w64 1246077374464), index:(w32 28), value:(w64 1240152408064)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_28(4);
  vector_table_1246077374464_value_28.set(0, 4, 1);
  state->vector_table_1246077374464.write(28, vector_table_1246077374464_value_28);
  // BDD node 130:vector_borrow(vector:(w64 1252519837696), index:(w32 28), val_out:(w64 1649829478400)[ -> (w64 1240085299200)])
  // Module Ignore
  // BDD node 131:vector_return(vector:(w64 1252519837696), index:(w32 28), value:(w64 1240085299200)[(w16 29)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_28(2);
  vector_register_1252519837696_value_28.set(0, 2, 29);
  state->vector_register_1252519837696.put(28, vector_register_1252519837696_value_28);
  // BDD node 132:vector_borrow(vector:(w64 1246077374464), index:(w32 29), val_out:(w64 1649812701184)[ -> (w64 1239118512128)])
  // Module Ignore
  // BDD node 133:vector_return(vector:(w64 1246077374464), index:(w32 29), value:(w64 1239118512128)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_29(4);
  vector_table_1246077374464_value_29.set(0, 4, 0);
  state->vector_table_1246077374464.write(29, vector_table_1246077374464_value_29);
  // BDD node 134:vector_borrow(vector:(w64 1252519837696), index:(w32 29), val_out:(w64 1649829478400)[ -> (w64 1239185620992)])
  // Module Ignore
  // BDD node 135:vector_return(vector:(w64 1252519837696), index:(w32 29), value:(w64 1239185620992)[(w16 28)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_29(2);
  vector_register_1252519837696_value_29.set(0, 2, 28);
  state->vector_register_1252519837696.put(29, vector_register_1252519837696_value_29);
  // BDD node 136:vector_borrow(vector:(w64 1246077374464), index:(w32 30), val_out:(w64 1649812701184)[ -> (w64 1240151359488)])
  // Module Ignore
  // BDD node 137:vector_return(vector:(w64 1246077374464), index:(w32 30), value:(w64 1240151359488)[(w32 1)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_30(4);
  vector_table_1246077374464_value_30.set(0, 4, 1);
  state->vector_table_1246077374464.write(30, vector_table_1246077374464_value_30);
  // BDD node 138:vector_borrow(vector:(w64 1252519837696), index:(w32 30), val_out:(w64 1649829478400)[ -> (w64 1240084250624)])
  // Module Ignore
  // BDD node 139:vector_return(vector:(w64 1252519837696), index:(w32 30), value:(w64 1240084250624)[(w16 31)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_30(2);
  vector_register_1252519837696_value_30.set(0, 2, 31);
  state->vector_register_1252519837696.put(30, vector_register_1252519837696_value_30);
  // BDD node 140:vector_borrow(vector:(w64 1246077374464), index:(w32 31), val_out:(w64 1649812701184)[ -> (w64 1239119560704)])
  // Module Ignore
  // BDD node 141:vector_return(vector:(w64 1246077374464), index:(w32 31), value:(w64 1239119560704)[(w32 0)])
  // Module DataplaneVectorTableUpdate
  buffer_t vector_table_1246077374464_value_31(4);
  vector_table_1246077374464_value_31.set(0, 4, 0);
  state->vector_table_1246077374464.write(31, vector_table_1246077374464_value_31);
  // BDD node 142:vector_borrow(vector:(w64 1252519837696), index:(w32 31), val_out:(w64 1649829478400)[ -> (w64 1239186669568)])
  // Module Ignore
  // BDD node 143:vector_return(vector:(w64 1252519837696), index:(w32 31), value:(w64 1239186669568)[(w16 30)])
  // Module DataplaneVectorRegisterUpdate
  buffer_t vector_register_1252519837696_value_31(2);
  vector_register_1252519837696_value_31.set(0, 2, 30);
  state->vector_register_1252519837696.put(31, vector_register_1252519837696_value_31);

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 time; // The switch's clock at the hand-off, ingress_mac_tstamp[47:16].
  u32 vector_data__160;
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

  // The packet as the NF sees it: without the cpu headers the data plane put in front of it.
  const u16 pkt_len = static_cast<u16>(size - packet_consumed);

  cpu_hdr->egress_dev = 0;
  cpu_hdr->trigger_dataplane_execution = 0;

  now = ((time_ns_t)bswap32(cpu_hdr_extra->time)) << 16;


  if (bswap16(cpu_hdr->code_path) == 0) {
    // EP node  1754
    // BDD node 161:vector_return(vector:(w64 1249835474944), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 255)), value:(w64 1239903371264)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__160))])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  1755
    // BDD node 161:vector_return(vector:(w64 1249835474944), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 255)), value:(w64 1239903371264)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__160))])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  1756
    // BDD node 161:vector_return(vector:(w64 1249835474944), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 255)), value:(w64 1239903371264)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__160))])
    u8* hdr_2 = packet_consume(pkt, 4);
    // EP node  1757
    // BDD node 161:vector_return(vector:(w64 1249835474944), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 255)), value:(w64 1239903371264)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__160))])
    buffer_t value_0;
    state->vector_table_1246077374464.read((u16)(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull), value_0);
    // EP node  1758
    // BDD node 161:vector_return(vector:(w64 1249835474944), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 255)), value:(w64 1239903371264)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__160))])
    if ((0) == ((u32)value_0.get(0, 4))) {
      // EP node  1759
      // BDD node 161:vector_return(vector:(w64 1249835474944), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 255)), value:(w64 1239903371264)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__160))])
      // EP node  3259
      // BDD node 161:vector_return(vector:(w64 1249835474944), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 255)), value:(w64 1239903371264)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__160))])
      buffer_t vector_register_1249835474944_value_0(4);
      vector_register_1249835474944_value_0.set(0, 4, (1) + (bswap32(cpu_hdr_extra->vector_data__160)));
      state->vector_register_1249835474944.put((*(u32*)(hdr_1 + 12)) & (255), vector_register_1249835474944_value_0);
      // EP node  3346
      // BDD node 162:vector_borrow(vector:(w64 1247687987200), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 65535)), val_out:(w64 1649854644224)[ -> (w64 1240134057984)])
      // EP node  3522
      // BDD node 163:vector_return(vector:(w64 1247687987200), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 65535)), value:(w64 1240134057984)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__162))])
      u8 *vector_cell_0;
      libnf::vector_borrow(state->cpu_vector_1247687987200, (*(u32*)(hdr_1 + 12)) & (65535), (void **)&vector_cell_0);
      u32 vector_value_0 = *(u32 *)vector_cell_0;
      vector_cell_0[0] = (u8)(((1) + (vector_value_0)));
      vector_cell_0[1] = (u8)(((1) + (vector_value_0))>>8);
      vector_cell_0[2] = (u8)(((1) + (vector_value_0))>>16);
      vector_cell_0[3] = (u8)(((1) + (vector_value_0))>>24);
      libnf::vector_return(state->cpu_vector_1247687987200, (*(u32*)(hdr_1 + 12)) & (65535), vector_cell_0);
      // EP node  3568
      // BDD node 164:crc32_hasher_hash(hasher:(w64 1245003644928), data:(w64 1649888198656)[(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 16777215))], size:(w32 4))
      buffer_t hash_data_0(4);
      hash_data_0[0] = *(u8*)(hdr_1 + 12);
      hash_data_0[1] = *(u8*)(hdr_1 + 13);
      hash_data_0[2] = *(u8*)(hdr_1 + 14);
      hash_data_0[3] = (u8)(((*(u32*)(hdr_1 + 12)) & (16777215))>>24);
      u32 hash_0 = state->crc32_hasher_1245003644928.hash(hash_data_0);
      // EP node  3660
      // BDD node 165:vector_inc_or_swap(vector:(w64 1251982962688), index:(And w32 (ReadLSB w32 (w32 0) hash__164) (w32 1023)), pair:(w64 1649888198656)[(Concat w64 (w8 0) (Concat w56 (w8 0) (Concat w48 (w8 0) (Concat w40 (w8 1) (And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 16777215)))))) -> (ReadLSB w64 (w32 0) pair_out__165)], key_size:(w32 4), evict:(w32 1), value_size:(w32 4))
      buffer_t pair_0(8);
      pair_0.set_little_endian(0, 4, (*(u32*)(hdr_1 + 12)) & (16777215));
      pair_0.set_little_endian(4, 1, 1);
      pair_0.set_little_endian(5, 1, 0);
      pair_0.set_little_endian(6, 1, 0);
      pair_0.set_little_endian(7, 1, 0);
      libnf::vector_inc_or_swap(state->cpu_vector_1251982962688, (hash_0) & (1023), pair_0.data, 4, 4, 1);
      // EP node  3754
      // BDD node 166:crc32_hasher_hash(hasher:(w64 1244735209472), data:(w64 1649888198656)[(ReadLSB w32 (w32 0) pair_out__165)], size:(w32 4))
      buffer_t hash_data_1(4);
      hash_data_1[0] = (u8)pair_0.get(0, 1);
      hash_data_1[1] = (u8)pair_0.get(1, 1);
      hash_data_1[2] = (u8)pair_0.get(2, 1);
      hash_data_1[3] = (u8)pair_0.get(3, 1);
      u32 hash_1 = state->crc32_hasher_1244735209472.hash(hash_data_1);
      // EP node  3850
      // BDD node 167:vector_inc_or_swap(vector:(w64 1248761733120), index:(And w32 (ReadLSB w32 (w32 0) hash__166) (w32 1023)), pair:(w64 1649888198656)[(ReadLSB w64 (w32 0) pair_out__165) -> (ReadLSB w64 (w32 0) pair_out__167)], key_size:(w32 4), evict:(w32 0), value_size:(w32 4))
      buffer_t pair_1(8);
      pair_1[0] = (u8)pair_0.get(0, 1);
      pair_1[1] = (u8)pair_0.get(1, 1);
      pair_1[2] = (u8)pair_0.get(2, 1);
      pair_1[3] = (u8)pair_0.get(3, 1);
      pair_1[4] = (u8)pair_0.get(4, 1);
      pair_1[5] = (u8)pair_0.get(5, 1);
      pair_1[6] = (u8)pair_0.get(6, 1);
      pair_1[7] = (u8)pair_0.get(7, 1);
      libnf::vector_inc_or_swap(state->cpu_vector_1248761733120, (hash_1) & (1023), pair_1.data, 4, 4, 0);
      // EP node  3948
      // BDD node 168:crc32_hasher_hash(hasher:(w64 1245272080384), data:(w64 1649888198656)[(ReadLSB w32 (w32 0) pair_out__167)], size:(w32 4))
      buffer_t hash_data_2(4);
      hash_data_2[0] = (u8)pair_1.get(0, 1);
      hash_data_2[1] = (u8)pair_1.get(1, 1);
      hash_data_2[2] = (u8)pair_1.get(2, 1);
      hash_data_2[3] = (u8)pair_1.get(3, 1);
      u32 hash_2 = state->crc32_hasher_1245272080384.hash(hash_data_2);
      // EP node  4048
      // BDD node 169:vector_inc_or_swap(vector:(w64 1246614245376), index:(And w32 (ReadLSB w32 (w32 0) hash__168) (w32 1023)), pair:(w64 1649888198656)[(ReadLSB w64 (w32 0) pair_out__167) -> (ReadLSB w64 (w32 0) pair_out__169)], key_size:(w32 4), evict:(w32 0), value_size:(w32 4))
      buffer_t pair_2(8);
      pair_2[0] = (u8)pair_1.get(0, 1);
      pair_2[1] = (u8)pair_1.get(1, 1);
      pair_2[2] = (u8)pair_1.get(2, 1);
      pair_2[3] = (u8)pair_1.get(3, 1);
      pair_2[4] = (u8)pair_1.get(4, 1);
      pair_2[5] = (u8)pair_1.get(5, 1);
      pair_2[6] = (u8)pair_1.get(6, 1);
      pair_2[7] = (u8)pair_1.get(7, 1);
      libnf::vector_inc_or_swap(state->cpu_vector_1246614245376, (hash_2) & (1023), pair_2.data, 4, 4, 0);
      // EP node  4150
      // BDD node 170:crc32_hasher_hash(hasher:(w64 1244600991744), data:(w64 1649888198656)[(ReadLSB w32 (w32 0) pair_out__169)], size:(w32 4))
      buffer_t hash_data_3(4);
      hash_data_3[0] = (u8)pair_2.get(0, 1);
      hash_data_3[1] = (u8)pair_2.get(1, 1);
      hash_data_3[2] = (u8)pair_2.get(2, 1);
      hash_data_3[3] = (u8)pair_2.get(3, 1);
      u32 hash_3 = state->crc32_hasher_1244600991744.hash(hash_data_3);
      // EP node  4254
      // BDD node 171:vector_inc_or_swap(vector:(w64 1253056708608), index:(And w32 (ReadLSB w32 (w32 0) hash__170) (w32 1023)), pair:(w64 1649888198656)[(ReadLSB w64 (w32 0) pair_out__169) -> (ReadLSB w64 (w32 0) pair_out__171)], key_size:(w32 4), evict:(w32 0), value_size:(w32 4))
      buffer_t pair_3(8);
      pair_3[0] = (u8)pair_2.get(0, 1);
      pair_3[1] = (u8)pair_2.get(1, 1);
      pair_3[2] = (u8)pair_2.get(2, 1);
      pair_3[3] = (u8)pair_2.get(3, 1);
      pair_3[4] = (u8)pair_2.get(4, 1);
      pair_3[5] = (u8)pair_2.get(5, 1);
      pair_3[6] = (u8)pair_2.get(6, 1);
      pair_3[7] = (u8)pair_2.get(7, 1);
      libnf::vector_inc_or_swap(state->cpu_vector_1253056708608, (hash_3) & (1023), pair_3.data, 4, 4, 0);
      // EP node  4360
      // BDD node 172:crc32_hasher_hash(hasher:(w64 1245406298112), data:(w64 1649888198656)[(ReadLSB w32 (w32 0) pair_out__171)], size:(w32 4))
      buffer_t hash_data_4(4);
      hash_data_4[0] = (u8)pair_3.get(0, 1);
      hash_data_4[1] = (u8)pair_3.get(1, 1);
      hash_data_4[2] = (u8)pair_3.get(2, 1);
      hash_data_4[3] = (u8)pair_3.get(3, 1);
      u32 hash_4 = state->crc32_hasher_1245406298112.hash(hash_data_4);
      // EP node  4468
      // BDD node 173:vector_inc_or_swap(vector:(w64 1250909220864), index:(And w32 (ReadLSB w32 (w32 0) hash__172) (w32 1023)), pair:(w64 1649888198656)[(ReadLSB w64 (w32 0) pair_out__171) -> (ReadLSB w64 (w32 0) pair_out__173)], key_size:(w32 4), evict:(w32 0), value_size:(w32 4))
      buffer_t pair_4(8);
      pair_4[0] = (u8)pair_3.get(0, 1);
      pair_4[1] = (u8)pair_3.get(1, 1);
      pair_4[2] = (u8)pair_3.get(2, 1);
      pair_4[3] = (u8)pair_3.get(3, 1);
      pair_4[4] = (u8)pair_3.get(4, 1);
      pair_4[5] = (u8)pair_3.get(5, 1);
      pair_4[6] = (u8)pair_3.get(6, 1);
      pair_4[7] = (u8)pair_3.get(7, 1);
      libnf::vector_inc_or_swap(state->cpu_vector_1250909220864, (hash_4) & (1023), pair_4.data, 4, 4, 0);
      // EP node  4578
      // BDD node 174:crc32_hasher_hash(hasher:(w64 1244869427200), data:(w64 1649888198656)[(ReadLSB w32 (w32 0) pair_out__173)], size:(w32 4))
      buffer_t hash_data_5(4);
      hash_data_5[0] = (u8)pair_4.get(0, 1);
      hash_data_5[1] = (u8)pair_4.get(1, 1);
      hash_data_5[2] = (u8)pair_4.get(2, 1);
      hash_data_5[3] = (u8)pair_4.get(3, 1);
      u32 hash_5 = state->crc32_hasher_1244869427200.hash(hash_data_5);
      // EP node  4690
      // BDD node 175:vector_inc_or_swap(vector:(w64 1248224862208), index:(And w32 (ReadLSB w32 (w32 0) hash__174) (w32 1023)), pair:(w64 1649888198656)[(ReadLSB w64 (w32 0) pair_out__173) -> (ReadLSB w64 (w32 0) pair_out__175)], key_size:(w32 4), evict:(w32 0), value_size:(w32 4))
      buffer_t pair_5(8);
      pair_5[0] = (u8)pair_4.get(0, 1);
      pair_5[1] = (u8)pair_4.get(1, 1);
      pair_5[2] = (u8)pair_4.get(2, 1);
      pair_5[3] = (u8)pair_4.get(3, 1);
      pair_5[4] = (u8)pair_4.get(4, 1);
      pair_5[5] = (u8)pair_4.get(5, 1);
      pair_5[6] = (u8)pair_4.get(6, 1);
      pair_5[7] = (u8)pair_4.get(7, 1);
      libnf::vector_inc_or_swap(state->cpu_vector_1248224862208, (hash_5) & (1023), pair_5.data, 4, 4, 0);
      // EP node  4747
      // BDD node 176:vector_borrow(vector:(w64 1252519837696), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), val_out:(w64 1650005639168)[ -> (w64 1240099979264)])
      buffer_t value_1;
      state->vector_register_1252519837696.get((u16)(bswap32(cpu_hdr_extra->DEVICE) & 0xffffull), value_1);
      // EP node  4805
      // BDD node 177:vector_return(vector:(w64 1252519837696), index:(ZExt w32 (ReadLSB w16 (w32 0) DEVICE)), value:(w64 1240099979264)[(ReadLSB w16 (w32 0) vector_data__176)])
      // EP node  5041
      // BDD node 181:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__176)))
      if ((bswap32(cpu_hdr_extra->DEVICE) & 0xffffull) != ((u16)value_1.get(0, 2))) {
        // EP node  5042
        // BDD node 181:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__176)))
        // EP node  5165
        // BDD node 182:FORWARD
        cpu_hdr->egress_dev = bswap16((u16)value_1.get(0, 2));
      } else {
        // EP node  5043
        // BDD node 181:if ((Eq false (Eq (ReadLSB w16 (w32 0) DEVICE) (ReadLSB w16 (w32 0) vector_data__176)))
        // EP node  5166
        // BDD node 183:DROP
        result.forward = false;
      }
    } else {
      // EP node  1760
      // BDD node 161:vector_return(vector:(w64 1249835474944), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 255)), value:(w64 1239903371264)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__160))])
      // EP node  1761
      // BDD node 161:vector_return(vector:(w64 1249835474944), index:(And w32 (ReadLSB w32 (w32 268) packet_chunks) (w32 255)), value:(w64 1239903371264)[(Add w32 (w32 1) (ReadLSB w32 (w32 0) vector_data__160))])
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
