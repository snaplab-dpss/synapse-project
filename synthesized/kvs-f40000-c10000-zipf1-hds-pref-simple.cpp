#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  MapTable map_table_1073923800;
  VectorRegister vector_register_1073954840;
  DchainTable dchain_table_1073971976;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      map_table_1073923800("map_table_1073923800",{"Ingress.map_table_1073923800_13",}, 1000LL),
      vector_register_1073954840("vector_register_1073954840",{"Ingress.vector_register_1073954840_0",}),
      dchain_table_1073971976("dchain_table_1073971976",{"Ingress.dchain_table_1073971976_38",}, 1000LL)
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
  // BDD node 0:map_allocate(capacity:(w32 8192), key_size:(w32 4), map_out:(w64 1073923528)[(w64 0) -> (w64 1073923800)])
  // Module DataplaneMapTableAllocate
  // BDD node 2:vector_allocate(elem_size:(w32 4), capacity:(w32 8192), vector_out:(w64 1073923544)[(w64 0) -> (w64 1073954840)])
  // Module DataplaneVectorRegisterAllocate
  // BDD node 3:dchain_allocate(index_range:(w32 8192), chain_out:(w64 1073923552)[ -> (w64 1073971976)])
  // Module DataplaneDchainTableAllocate

}

void sycon::nf_exit() {

}

void sycon::nf_args(CLI::App &app) {

}

void sycon::nf_user_signal_handler() {

}

struct cpu_hdr_extra_t {
  u32 map_has_this_key;
  u32 DEVICE;
  u64 next_time;

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

  if (bswap16(cpu_hdr->code_path) == 2329) {
    // EP node  2312
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  2313
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  2314
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 8);
    // EP node  2315
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_3 = packet_consume(pkt, 12);
    // EP node  2316
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) != (bswap32(cpu_hdr_extra->DEVICE) & 65535)) {
      // EP node  2317
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  2320
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      buffer_t map_table_1073923800_key_0(4);
      map_table_1073923800_key_0[0] = *(u8*)(hdr_3 + 1);
      map_table_1073923800_key_0[1] = *(u8*)(hdr_3 + 2);
      map_table_1073923800_key_0[2] = *(u8*)(hdr_3 + 3);
      map_table_1073923800_key_0[3] = *(u8*)(hdr_3 + 4);
      u32 value_0;
      bool found_0 = state->map_table_1073923800.get(map_table_1073923800_key_0, value_0);
      // EP node  2321
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_0)) {
        // EP node  2322
        // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  2325
        // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        if ((1) == (*(u8*)(hdr_3 + 0))) {
          // EP node  2326
          // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          // EP node  6627
          // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          u32 allocated_index_0;
          bool success_0 = state->dchain_table_1073971976.allocate_new_index(allocated_index_0);
          // EP node  6694
          // BDD node 17:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
          if ((0) == (success_0)) {
            // EP node  6695
            // BDD node 17:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
            // EP node  6834
            // BDD node 18:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 1) DEVICE) (Concat w88 (Read w8 (w32 0) DEVICE) (ReadLSB w80 (w32 768) packet_chunks)))])
            hdr_3[10] = bswap32(cpu_hdr_extra->DEVICE) & 255;
            hdr_3[11] = (bswap32(cpu_hdr_extra->DEVICE)>>8) & 255;
            // EP node  7119
            // BDD node 22:FORWARD
            cpu_hdr->egress_dev = bswap16(0);
          } else {
            // EP node  6696
            // BDD node 17:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
            // EP node  7192
            // BDD node 26:vector_borrow(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) new_index), val_out:(w64 1074042440)[ -> (w64 1073968736)])
            // EP node  7411
            // BDD node 27:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) new_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
            buffer_t vector_register_1073954840_value_0(4);
            vector_register_1073954840_value_0[0] = *(u8*)(hdr_3 + 5);
            vector_register_1073954840_value_0[1] = *(u8*)(hdr_3 + 6);
            vector_register_1073954840_value_0[2] = *(u8*)(hdr_3 + 7);
            vector_register_1073954840_value_0[3] = *(u8*)(hdr_3 + 8);
            state->vector_register_1073954840.put(allocated_index_0, vector_register_1073954840_value_0);
            // EP node  7412
            // BDD node 24:map_put(map:(w64 1073923800), key:(w64 1073951520)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value:(ReadLSB w32 (w32 0) new_index))
            buffer_t map_table_1073923800_key_1(4);
            map_table_1073923800_key_1[0] = *(u8*)(hdr_3 + 1);
            map_table_1073923800_key_1[1] = *(u8*)(hdr_3 + 2);
            map_table_1073923800_key_1[2] = *(u8*)(hdr_3 + 3);
            map_table_1073923800_key_1[3] = *(u8*)(hdr_3 + 4);
            state->map_table_1073923800.put(map_table_1073923800_key_1, allocated_index_0);
            // EP node  7562
            // BDD node 28:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (ReadLSB w72 (w32 768) packet_chunks))))])
            hdr_3[9] = 1;
            // EP node  7639
            // BDD node 29:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760672)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
            std::swap(hdr_2[0], hdr_2[2]);
            std::swap(hdr_2[1], hdr_2[3]);
            // EP node  7717
            // BDD node 30:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760416)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
            std::swap(hdr_1[12], hdr_1[16]);
            std::swap(hdr_1[13], hdr_1[17]);
            std::swap(hdr_1[14], hdr_1[18]);
            std::swap(hdr_1[15], hdr_1[19]);
            // EP node  7796
            // BDD node 31:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760160)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
            std::swap(hdr_0[0], hdr_0[6]);
            std::swap(hdr_0[1], hdr_0[7]);
            std::swap(hdr_0[2], hdr_0[8]);
            std::swap(hdr_0[3], hdr_0[9]);
            std::swap(hdr_0[4], hdr_0[10]);
            std::swap(hdr_0[5], hdr_0[11]);
            // EP node  7876
            // BDD node 32:FORWARD
            cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
          }
        } else {
          // EP node  2327
          // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          // EP node  2328
          // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          result.abort_transaction = true;
          cpu_hdr->trigger_dataplane_execution = 1;
          return result;
        }
      } else {
        // EP node  2323
        // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  2324
        // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  2318
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  2319
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
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
