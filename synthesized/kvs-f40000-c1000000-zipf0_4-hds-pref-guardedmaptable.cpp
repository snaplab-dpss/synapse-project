#include <sycon/sycon.h>

using namespace sycon;

struct state_t : public nf_state_t {
  IngressPortToNFDev ingress_port_to_nf_dev;
  ForwardingTbl forwarding_tbl;
  GuardedMapTable guarded_map_table_1073923800;
  VectorTable vector_table_1073954840;
  DchainTable dchain_table_1073971976;

  state_t()
    : ingress_port_to_nf_dev(),
      forwarding_tbl(),
      guarded_map_table_1073923800("guarded_map_table_1073923800",{"Ingress.guarded_map_table_1073923800_13",},"Ingress.guarded_map_table_1073923800_guard", 1000LL),
      vector_table_1073954840("vector_table_1073954840",{"Ingress.vector_table_1073954840_124",}),
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
  // Module DataplaneGuardedMapTableAllocate
  // BDD node 2:vector_allocate(elem_size:(w32 4), capacity:(w32 8192), vector_out:(w64 1073923544)[(w64 0) -> (w64 1073954840)])
  // Module DataplaneVectorTableAllocate
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

  if (bswap16(cpu_hdr->code_path) == 4248) {
    // EP node  4226
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_0 = packet_consume(pkt, 14);
    // EP node  4227
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_1 = packet_consume(pkt, 20);
    // EP node  4228
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_2 = packet_consume(pkt, 8);
    // EP node  4229
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    u8* hdr_3 = packet_consume(pkt, 12);
    // EP node  4230
    // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
    if ((0) != (bswap32(cpu_hdr_extra->DEVICE) & 65535)) {
      // EP node  4231
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  4234
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      buffer_t guarded_map_table_1073923800_key_0(4);
      guarded_map_table_1073923800_key_0[0] = *(u8*)(hdr_3 + 1);
      guarded_map_table_1073923800_key_0[1] = *(u8*)(hdr_3 + 2);
      guarded_map_table_1073923800_key_0[2] = *(u8*)(hdr_3 + 3);
      guarded_map_table_1073923800_key_0[3] = *(u8*)(hdr_3 + 4);
      u32 value_0;
      bool found_0 = state->guarded_map_table_1073923800.get(guarded_map_table_1073923800_key_0, value_0);
      // EP node  4235
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      if ((0) == (found_0)) {
        // EP node  4236
        // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  4239
        // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        if ((1) == (*(u8*)(hdr_3 + 0))) {
          // EP node  4240
          // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          // EP node  4243
          // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          bool guard_allow_0 = state->guarded_map_table_1073923800.guard_check();
          // EP node  4244
          // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          if ((guard_allow_0) != (0)) {
            // EP node  4245
            // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
            // EP node  10651
            // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
            u32 allocated_index_0;
            bool success_0 = state->dchain_table_1073971976.allocate_new_index(allocated_index_0);
            // EP node  10750
            // BDD node 17:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
            if ((0) == (success_0)) {
              // EP node  10751
              // BDD node 17:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
              // EP node  11916
              // BDD node 18:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 1) DEVICE) (Concat w88 (Read w8 (w32 0) DEVICE) (ReadLSB w80 (w32 768) packet_chunks)))])
              hdr_3[10] = bswap32(cpu_hdr_extra->DEVICE) & 255;
              hdr_3[11] = (bswap32(cpu_hdr_extra->DEVICE)>>8) & 255;
              // EP node  12361
              // BDD node 22:FORWARD
              cpu_hdr->egress_dev = bswap16(0);
            } else {
              // EP node  10752
              // BDD node 17:if ((Eq (w32 0) (ReadLSB w32 (w32 0) not_out_of_space_2))
              // EP node  10954
              // BDD node 24:map_put(map:(w64 1073923800), key:(w64 1073951520)[(ReadLSB w32 (w32 769) packet_chunks) -> (ReadLSB w32 (w32 769) packet_chunks)], value:(ReadLSB w32 (w32 0) new_index))
              buffer_t guarded_map_table_1073923800_key_1(4);
              guarded_map_table_1073923800_key_1[0] = *(u8*)(hdr_3 + 1);
              guarded_map_table_1073923800_key_1[1] = *(u8*)(hdr_3 + 2);
              guarded_map_table_1073923800_key_1[2] = *(u8*)(hdr_3 + 3);
              guarded_map_table_1073923800_key_1[3] = *(u8*)(hdr_3 + 4);
              state->guarded_map_table_1073923800.put(guarded_map_table_1073923800_key_1, allocated_index_0);
              // EP node  11057
              // BDD node 26:vector_borrow(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) new_index), val_out:(w64 1074042440)[ -> (w64 1073968736)])
              // EP node  11161
              // BDD node 27:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) new_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
              buffer_t vector_table_1073954840_value_0(4);
              vector_table_1073954840_value_0[0] = *(u8*)(hdr_3 + 5);
              vector_table_1073954840_value_0[1] = *(u8*)(hdr_3 + 6);
              vector_table_1073954840_value_0[2] = *(u8*)(hdr_3 + 7);
              vector_table_1073954840_value_0[3] = *(u8*)(hdr_3 + 8);
              state->vector_table_1073954840.write(allocated_index_0, vector_table_1073954840_value_0);
              // EP node  11371
              // BDD node 28:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (ReadLSB w72 (w32 768) packet_chunks))))])
              hdr_3[9] = 1;
              // EP node  11478
              // BDD node 29:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760672)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
              std::swap(hdr_2[0], hdr_2[2]);
              std::swap(hdr_2[1], hdr_2[3]);
              // EP node  11586
              // BDD node 30:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760416)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
              std::swap(hdr_1[12], hdr_1[16]);
              std::swap(hdr_1[13], hdr_1[17]);
              std::swap(hdr_1[14], hdr_1[18]);
              std::swap(hdr_1[15], hdr_1[19]);
              // EP node  11695
              // BDD node 31:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760160)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
              std::swap(hdr_0[0], hdr_0[6]);
              std::swap(hdr_0[1], hdr_0[7]);
              std::swap(hdr_0[2], hdr_0[8]);
              std::swap(hdr_0[3], hdr_0[9]);
              std::swap(hdr_0[4], hdr_0[10]);
              std::swap(hdr_0[5], hdr_0[11]);
              // EP node  11805
              // BDD node 32:FORWARD
              cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
            }
          } else {
            // EP node  4246
            // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
            // EP node  4247
            // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
            result.abort_transaction = true;
            cpu_hdr->trigger_dataplane_execution = 1;
            return result;
          }
        } else {
          // EP node  4241
          // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          // EP node  4242
          // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
          result.abort_transaction = true;
          cpu_hdr->trigger_dataplane_execution = 1;
          return result;
        }
      } else {
        // EP node  4237
        // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        // EP node  4238
        // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      }
    } else {
      // EP node  4232
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      // EP node  4233
      // BDD node 16:dchain_allocate_new_index(chain:(w64 1073971976), index_out:(w64 1074042384)[(w32 2880154539) -> (ReadLSB w32 (w32 0) new_index)], time:(ReadLSB w64 (w32 0) next_time))
      result.abort_transaction = true;
      cpu_hdr->trigger_dataplane_execution = 1;
      return result;
    }
  }
  else if (bswap16(cpu_hdr->code_path) == 3782) {
    // EP node  3765
    // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
    u8* hdr_4 = packet_consume(pkt, 14);
    // EP node  3766
    // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
    u8* hdr_5 = packet_consume(pkt, 20);
    // EP node  3767
    // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
    u8* hdr_6 = packet_consume(pkt, 8);
    // EP node  3768
    // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
    u8* hdr_7 = packet_consume(pkt, 12);
    // EP node  3769
    // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
    if ((0) != (bswap32(cpu_hdr_extra->DEVICE) & 65535)) {
      // EP node  3770
      // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
      // EP node  3773
      // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
      buffer_t guarded_map_table_1073923800_key_2(4);
      guarded_map_table_1073923800_key_2[0] = *(u8*)(hdr_7 + 1);
      guarded_map_table_1073923800_key_2[1] = *(u8*)(hdr_7 + 2);
      guarded_map_table_1073923800_key_2[2] = *(u8*)(hdr_7 + 3);
      guarded_map_table_1073923800_key_2[3] = *(u8*)(hdr_7 + 4);
      u32 value_1;
      bool found_1 = state->guarded_map_table_1073923800.get(guarded_map_table_1073923800_key_2, value_1);
      // EP node  3774
      // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
      if ((0) == (found_1)) {
        // EP node  3775
        // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
        // EP node  3777
        // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
        result.abort_transaction = true;
        cpu_hdr->trigger_dataplane_execution = 1;
        return result;
      } else {
        // EP node  3776
        // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
        // EP node  3778
        // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
        if ((1) == (*(u8*)(hdr_7 + 0))) {
          // EP node  3779
          // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
          // EP node  10078
          // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
          buffer_t vector_table_1073954840_value_1(4);
          vector_table_1073954840_value_1[0] = *(u8*)(hdr_7 + 5);
          vector_table_1073954840_value_1[1] = *(u8*)(hdr_7 + 6);
          vector_table_1073954840_value_1[2] = *(u8*)(hdr_7 + 7);
          vector_table_1073954840_value_1[3] = *(u8*)(hdr_7 + 8);
          state->vector_table_1073954840.write(value_1, vector_table_1073954840_value_1);
          // EP node  10264
          // BDD node 42:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760928)[(Concat w96 (Read w8 (w32 779) packet_chunks) (Concat w88 (Read w8 (w32 778) packet_chunks) (Concat w80 (w8 1) (ReadLSB w72 (w32 768) packet_chunks))))])
          hdr_7[9] = 1;
          // EP node  10359
          // BDD node 43:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760672)[(Concat w64 (Read w8 (w32 519) packet_chunks) (Concat w56 (Read w8 (w32 518) packet_chunks) (Concat w48 (Read w8 (w32 517) packet_chunks) (Concat w40 (Read w8 (w32 516) packet_chunks) (Concat w32 (Read w8 (w32 513) packet_chunks) (Concat w24 (Read w8 (w32 512) packet_chunks) (ReadLSB w16 (w32 514) packet_chunks)))))))])
          std::swap(hdr_6[0], hdr_6[2]);
          std::swap(hdr_6[1], hdr_6[3]);
          // EP node  10455
          // BDD node 44:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760416)[(Concat w160 (Read w8 (w32 271) packet_chunks) (Concat w152 (Read w8 (w32 270) packet_chunks) (Concat w144 (Read w8 (w32 269) packet_chunks) (Concat w136 (Read w8 (w32 268) packet_chunks) (Concat w128 (Read w8 (w32 275) packet_chunks) (Concat w120 (Read w8 (w32 274) packet_chunks) (Concat w112 (Read w8 (w32 273) packet_chunks) (Concat w104 (Read w8 (w32 272) packet_chunks) (ReadLSB w96 (w32 256) packet_chunks)))))))))])
          std::swap(hdr_5[12], hdr_5[16]);
          std::swap(hdr_5[13], hdr_5[17]);
          std::swap(hdr_5[14], hdr_5[18]);
          std::swap(hdr_5[15], hdr_5[19]);
          // EP node  10552
          // BDD node 45:packet_return_chunk(p:(w64 1074028624), the_chunk:(w64 1073760160)[(Concat w112 (Read w8 (w32 13) packet_chunks) (Concat w104 (Read w8 (w32 12) packet_chunks) (Concat w96 (Read w8 (w32 5) packet_chunks) (Concat w88 (Read w8 (w32 4) packet_chunks) (Concat w80 (Read w8 (w32 3) packet_chunks) (Concat w72 (Read w8 (w32 2) packet_chunks) (Concat w64 (Read w8 (w32 1) packet_chunks) (Concat w56 (Read w8 (w32 0) packet_chunks) (ReadLSB w48 (w32 6) packet_chunks)))))))))])
          std::swap(hdr_4[0], hdr_4[6]);
          std::swap(hdr_4[1], hdr_4[7]);
          std::swap(hdr_4[2], hdr_4[8]);
          std::swap(hdr_4[3], hdr_4[9]);
          std::swap(hdr_4[4], hdr_4[10]);
          std::swap(hdr_4[5], hdr_4[11]);
          // EP node  10650
          // BDD node 46:FORWARD
          cpu_hdr->egress_dev = bswap16(bswap32(cpu_hdr_extra->DEVICE) & 65535);
        } else {
          // EP node  3780
          // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
          // EP node  3781
          // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
          result.abort_transaction = true;
          cpu_hdr->trigger_dataplane_execution = 1;
          return result;
        }
      }
    } else {
      // EP node  3771
      // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
      // EP node  3772
      // BDD node 41:vector_return(vector:(w64 1073954840), index:(ReadLSB w32 (w32 0) allocated_index), value:(w64 1073968736)[(ReadLSB w32 (w32 773) packet_chunks)])
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
