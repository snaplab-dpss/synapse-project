#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardNFDev forward_nf_dev;
  HHTable hh_table_1073923160;
  VectorTable vector_table_1073954200;

  state_t()
    : ingress_port_to_nf_dev(),
      forward_nf_dev(),
      hh_table_1073923160("hh_table_1073923160",{"Ingress.hh_table_1073923160_table_13", }, "Ingress.hh_table_1073923160_cached_counters", {"Ingress.hh_table_1073923160_cms_row_0", "Ingress.hh_table_1073923160_cms_row_1", "Ingress.hh_table_1073923160_cms_row_2", "Ingress.hh_table_1073923160_cms_row_3", }, "Ingress.hh_table_1073923160_threshold", "IngressDeparser.hh_table_1073923160_digest", 100LL),
      vector_table_1073954200("vector_table_1073954200",{"Ingress.vector_table_1073954200_46",})
    {}
};

state_t *state = nullptr;

void sycon::nf_init() {
  nf_state = std::make_unique<state_t>();
  state    = dynamic_cast<state_t *>(nf_state.get());
  state->ingress_port_to_nf_dev.add_entry(asic_get_dev_port(1), 0);
  state->forward_nf_dev.add_entry(0, asic_get_dev_port(1));
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
  // BDD node 0:map_allocate(capacity:(w32 8192), key_size:(w32 4), map_out:(w64 1073922888)[(w64 0) -> (w64 1073923160)])
  // Module DataplaneHHTableAllocate
  // BDD node 2:vector_allocate(elem_size:(w32 4), capacity:(w32 8192), vector_out:(w64 1073922904)[(w64 0) -> (w64 1073954200)])
  // Module DataplaneVectorTableAllocate
  // BDD node 3:dchain_allocate(index_range:(w32 8192), chain_out:(w64 1073922912)[ -> (w64 1073971336)])
  // Module Ignore

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u8 packet_chunks[1280];
  u32 map_has_this_key;
  u32 allocated_index;
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

  if (bswap16(cpu_hdr->code_path) == 4100) {
    // EP node  4083
    // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  4084
    // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  4085
    // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
    u8* hdr_2 = packet_consume(pkt, 8);
    // EP node  4086
    // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
    u8* hdr_3 = packet_consume(pkt, 12);
    // EP node  4087
    // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
    if ((0) != (bswap32(cpu_hdr_extra->DEVICE) & 65535)) {
      // EP node  4088
      // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
      // EP node  4091
      // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
            buffer_t hh_table_1073923160_key_0(4);
      hh_table_1073923160_key_0[0] = *(u8*)(hdr_3 + 1);
      hh_table_1073923160_key_0[1] = *(u8*)(hdr_3 + 2);
      hh_table_1073923160_key_0[2] = *(u8*)(hdr_3 + 3);
      hh_table_1073923160_key_0[3] = *(u8*)(hdr_3 + 4);
      u32 value_0;
      bool found_0 = state->hh_table_1073923160.get(hh_table_1073923160_key_0, value_0);
      // EP node  4092
      // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
      if ((0) == (found_0)) {
        // EP node  4093
        // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
        // EP node  4095
        // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      } else {
        // EP node  4094
        // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
        // EP node  4096
        // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
        if ((1) != (*(u8*)(hdr_3 + 0))) {
          // EP node  4097
          // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
          // EP node  4099
          // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
          result.abort_transaction = true;
          cpu_hdr->trigger_dataplane_execution = 1;
          return result;
        } else {
          // EP node  4098
          // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
          // EP node  10560
          // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
          buffer_t vector_table_1073954200_value_0(4);
          vector_table_1073954200_value_0[0] = *(u8*)(hdr_3 + 5);
          vector_table_1073954200_value_0[1] = *(u8*)(hdr_3 + 6);
          vector_table_1073954200_value_0[2] = *(u8*)(hdr_3 + 7);
          vector_table_1073954200_value_0[3] = *(u8*)(hdr_3 + 8);
          state->vector_table_1073954200.write(value_0, vector_table_1073954200_value_0);
          // EP node  10706
          // BDD node 55:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073760288)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (ReadLSB w72 (w32 768) packet_chunks))))])
          hdr_3[9] = 1;
          // EP node  10781
          // BDD node 56:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073760032)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
          std::swap(hdr_2[0], hdr_2[2]);
          std::swap(hdr_2[1], hdr_2[3]);
          // EP node  10857
          // BDD node 57:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073759776)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
          std::swap(hdr_1[12], hdr_1[16]);
          std::swap(hdr_1[13], hdr_1[17]);
          std::swap(hdr_1[14], hdr_1[18]);
          std::swap(hdr_1[15], hdr_1[19]);
          // EP node  10934
          // BDD node 58:packet_return_chunk(p:(w64 1074031024), the_chunk:(w64 1073759520)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
          std::swap(hdr_0[0], hdr_0[6]);
          std::swap(hdr_0[1], hdr_0[7]);
          std::swap(hdr_0[2], hdr_0[8]);
          std::swap(hdr_0[3], hdr_0[9]);
          std::swap(hdr_0[4], hdr_0[10]);
          std::swap(hdr_0[5], hdr_0[11]);
          // EP node  11012
          // BDD node 59:FORWARD
          cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
        }
      }
    } else {
      // EP node  4089
      // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
      // EP node  4090
      // BDD node 54:vector_return(vector:(w64 1073954200), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968096)[(ReadLSB w32 (w32 773) packet_chunks)])
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
