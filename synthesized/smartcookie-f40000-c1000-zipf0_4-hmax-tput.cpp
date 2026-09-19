#include <sycon/sycon.h>
#include <sycon/libnf.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  BloomFilter bf_1073927040;
  VectorTable vector_table_1073939616;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      bf_1073927040("bf_1073927040",{"Ingress.bf_1073927040_row_0", "Ingress.bf_1073927040_row_1", }, 0LL),
      vector_table_1073939616("vector_table_1073939616",{"Ingress.vector_table_1073939616_105","Ingress.vector_table_1073939616_88",})
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

  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(1), 0);
  state->forwarding_tbl.add_fwd_nf_dev_entry(0, asic_get_dev_port(1));
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
  // BDD node 0:bf_allocate(height:(w32 2), width:(w32 1048576), key_size:(w16 12), cleanup_interval:(w64 0), bf_out:(w64 1073926768)[(w64 0) -> (w64 1073927040)])
  // Module DataplaneBloomFilterAllocate
  // BDD node 1:vector_allocate(elem_size:(w32 4), capacity:(w32 1), vector_out:(w64 1073926776)[(w64 0) -> (w64 1073939616)])
  // Module DataplaneVectorTableAllocate

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 time; // The switch's clock at the hand-off, ingress_mac_tstamp[47:16].
  u64 unrolled__226;
  // The data plane's state header, as it follows the cpu header on every packet.
  u32 st_s32_1;
  u32 st_s32_2;
  u32 st_s32_3;
  u32 st_s32_4;
  u32 st_s32_5;
  u32 st_s32_6;

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
    // EP node  730472
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  730473
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  730474
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_2 = packet_consume(pkt, 8);
    // EP node  730475
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    u8* hdr_3 = packet_consume(pkt, 4);
    // EP node  730477
    // BDD node 212:vector_return(vector:(w64 1073939616), index:(w32 0), value:(w64 1073953512)[(Sub w32 (ReadLSB w32 (w32 0) unrolled__226) (Concat w32 (Concat w24 (ReadMSB w16 (w32 768) packet_chunks) (Read w8 (w32 770) packet_chunks)) (Read w8 (w32 771) packet_chunks)))])
    buffer_t vector_table_1073939616_value_0(4);
    vector_table_1073939616_value_0.set(0, 4, (bswap64(cpu_hdr_extra->unrolled__226) & 0xffffffffull) - (bswap32(*(u32*)hdr_3)));
    state->vector_table_1073939616.write(0, vector_table_1073939616_value_0);
    // EP node  732782
    // BDD node 217:DROP
    result.forward = false;
  }


  if (trigger_update_ipv4_tcpudp_checksums) {
    update_ipv4_tcpudp_checksums(l3_hdr, l4_hdr);
  }

  return result;
}

int main(int argc, char **argv) { SYNAPSE_CONTROLLER_MAIN(argc, argv) }
