#include <LibCore/TrafficGenerator.h>

namespace LibCore {

TrafficGenerator::TrafficGenerator(const std::string &_nf, const config_t &_config)
    : nf(_nf), config(_config), lan_to_wan_dev(build_lan_to_wan(config.lan_wan_pairs)),
      wan_to_lan_dev(build_wan_to_lan(config.lan_wan_pairs)), lan_devices(build_lan_devices(config.lan_wan_pairs)),
      wan_devices(build_wan_devices(config.lan_wan_pairs)), template_packet(build_pkt_template()),
      uniform_rand(_config.random_seed, 0, _config.total_flows - 1),
      zipf_rand(_config.random_seed, _config.zipf_param, _config.total_flows), pd(NULL), pdumper(NULL), current_lan_dev_it(0), counters(0),
      flows_swapped(0), current_time(0), alarm_tick(0), next_alarm(-1) {
  for (u16 lan_dev : lan_devices) {
    lan_writers.emplace(lan_dev, get_pcap_fname(nf, config, lan_dev));
    lan_warmup_writers.emplace(lan_dev, get_warmup_pcap_fname(nf, config, lan_dev));
  }

  for (u16 wan_dev : wan_devices) {
    wan_writers.emplace(wan_dev, get_pcap_fname(nf, config, wan_dev));
  }

  for (flow_idx_t i = 0; i < config.total_flows; i++) {
    u16 lan_dev = get_current_lan_dev();
    advance_lan_dev();

    flows_dev_turn[i]   = Dev::LAN;
    flows_to_lan_dev[i] = lan_dev;
    counters[i]         = 0;
  }

  if (config.churn_fpm > 0) {
    alarm_tick = 60'000'000'000 / config.churn_fpm;
    next_alarm = alarm_tick;
  }

  reset_lan_dev();
}

void TrafficGenerator::config_t::print() const {
  printf("----- Config -----\n");
  printf("out dir:     %s\n", out_dir.c_str());
  printf("#packets:    %lu\n", total_packets);
  printf("#flows:      %lu\n", total_flows);
  printf("packet size: %u\n", packet_size);
  printf("rate:        %lu mbps\n", rate / 1'000'000);
  printf("churn:       %lu fpm\n", churn_fpm);
  switch (traffic_type) {
  case TrafficType::Uniform:
    printf("traffic:     uniform\n");
    break;
  case TrafficType::Zipf:
    printf("traffic:     zipf\n");
    printf("zipf param:  %f\n", zipf_param);
    break;
  }
  printf("LAN/WAN pairs:\n");
  for (const auto &[lan, wan] : lan_wan_pairs) {
    printf("  %d -> %d\n", lan, wan);
  }
  printf("random seed: %u\n", random_seed);
  printf("--- ---------- ---\n");
}

TrafficGenerator::pkt_t TrafficGenerator::build_pkt_template() const {
  pkt_t pkt;

  pkt.eth_hdr.ether_type = htons(ETHERTYPE_IP);
  LibCore::parse_etheraddr(DMAC, &pkt.eth_hdr.daddr);
  LibCore::parse_etheraddr(SMAC, &pkt.eth_hdr.saddr);

  pkt.ip_hdr.version         = 4;
  pkt.ip_hdr.ihl             = 5;
  pkt.ip_hdr.type_of_service = 0;
  pkt.ip_hdr.total_length    = htons(config.packet_size - sizeof(pkt.eth_hdr));
  pkt.ip_hdr.packet_id       = 0;
  pkt.ip_hdr.fragment_offset = 0;
  pkt.ip_hdr.time_to_live    = 64;
  pkt.ip_hdr.next_proto_id   = IPPROTO_UDP;
  pkt.ip_hdr.hdr_checksum    = 0;
  pkt.ip_hdr.src_addr        = 0;
  pkt.ip_hdr.dst_addr        = 0;

  pkt.udp_hdr.src_port = 0;
  pkt.udp_hdr.dst_port = 0;
  pkt.udp_hdr.len      = htons(config.packet_size - (sizeof(pkt.eth_hdr) + sizeof(pkt.ip_hdr) + sizeof(pkt.udp_hdr)));
  pkt.udp_hdr.checksum = 0;

  memset(pkt.payload, 0x42, sizeof(pkt.payload));

  return pkt;
}

void TrafficGenerator::generate() {
  u64 counter  = 0;
  u64 goal     = config.total_packets;
  int progress = -1;

  for (const auto &[lan_dev, lan_writer] : lan_writers) {
    printf("LAN dev %u: %s\n", lan_dev, lan_writer.get_output_fname().c_str());
  }

  for (const auto &[wan_dev, wan_writer] : wan_writers) {
    printf("WAN dev %u: %s\n", wan_dev, wan_writer.get_output_fname().c_str());
  }

  // FIXME: this is too slow! we should be sending at all ports at the same time.
  for (u64 i = 0; i < config.total_packets; i++) {
    if (next_alarm >= 0 && current_time >= next_alarm) {
      u16 lan_dev = get_current_lan_dev();
      advance_lan_dev();

      flow_idx_t chosen_swap_flow_idx = uniform_rand.generate();
      random_swap_flow(chosen_swap_flow_idx);
      flows_swapped++;
      next_alarm += alarm_tick;

      flows_dev_turn[chosen_swap_flow_idx]   = Dev::LAN;
      flows_to_lan_dev[chosen_swap_flow_idx] = lan_dev;
      counters[chosen_swap_flow_idx]         = 0;
    }

    flow_idx_t flow_idx{0};
    switch (config.traffic_type) {
    case TrafficType::Uniform:
      flow_idx = uniform_rand.generate();
      break;
    case TrafficType::Zipf:
      flow_idx = zipf_rand.generate();
      break;
    }
    assert(flow_idx < config.total_flows);

    if (flows_dev_turn[flow_idx] == Dev::LAN) {
      u16 lan_dev = flows_to_lan_dev.at(flow_idx);
      if (expects_response(lan_dev, flow_idx)) {
        flows_dev_turn[flow_idx] = Dev::WAN;
      }

      pkt_t pkt = build_lan_packet(lan_dev, flow_idx);
      lan_writers.at(lan_dev).write((const u8 *)&pkt, config.packet_size, config.packet_size, current_time);
    } else {
      u16 lan_dev              = flows_to_lan_dev.at(flow_idx);
      u16 wan_dev              = lan_to_wan_dev.at(lan_dev);
      flows_dev_turn[flow_idx] = Dev::LAN;

      pkt_t pkt = build_wan_packet(wan_dev, flow_idx);
      wan_writers.at(wan_dev).write((const u8 *)&pkt, config.packet_size, config.packet_size, current_time);
    }

    counters[flow_idx]++;

    tick();

    counter++;
    int current_progress = (counter * 100) / goal;

    if (current_progress > progress) {
      progress = current_progress;
      printf("\r  Progress: %3d%%", progress);
      fflush(stdout);
    }
  }

  printf("\n");

  report();
}

void TrafficGenerator::report() const {
  std::vector<u64> counters_values;
  counters_values.reserve(counters.size());
  for (const auto &kv : counters) {
    // Take warmup into account.
    counters_values.push_back(kv.second);
  }
  std::sort(counters_values.begin(), counters_values.end(), std::greater{});

  u64 total_flows = counters.size();

  u64 hh         = 0;
  u64 hh_packets = 0;
  for (size_t i = 0; i < total_flows; i++) {
    hh++;
    hh_packets += counters_values[i];
    if (hh_packets >= config.total_packets * 0.8) {
      break;
    }
  }

  printf("\n");
  printf("Base flows: %ld\n", config.total_flows);
  printf("Total flows: %ld\n", total_flows);
  printf("Swapped flows: %ld\n", flows_swapped);
  printf("HH: %ld flows (%.2f%%) %.2f%% volume\n", hh, 100.0 * hh / total_flows, 100.0 * hh_packets / config.total_packets);
  printf("Top 10 flows:\n");
  for (size_t i = 0; i < config.total_flows; i++) {
    printf("  flow %ld: %ld\n", i, counters_values[i]);

    if (i >= 9) {
      break;
    }
  }
}

void TrafficGenerator::generate_warmup() {
  u64 counter  = 0;
  u64 goal     = config.total_flows;
  int progress = -1;

  for (const auto &[_, warmup_writer] : lan_warmup_writers) {
    printf("Warmup: %s\n", warmup_writer.get_output_fname().c_str());
  }

  for (auto &[lan_dev, warmup_writer] : lan_warmup_writers) {
    for (const auto &[flow_idx, dev] : flows_to_lan_dev) {
      if (dev != lan_dev) {
        continue;
      }

      pkt_t pkt = build_lan_packet(dev, flow_idx);
      warmup_writer.write((const u8 *)&pkt, config.packet_size, config.packet_size, current_time);

      counter++;
      int current_progress = (counter * 100) / goal;

      if (current_progress > progress) {
        progress = current_progress;
        printf("\r  Progress: %3d%%", progress);
        fflush(stdout);
      }
    }
  }

  printf("\n");
}

} // namespace LibCore