#include <LibCore/TrafficGenerator.h>

namespace LibCore {

TrafficGenerator::TrafficGenerator(const std::string &_nf, const config_t &_config, bool _assume_ip)
    : nf(_nf), config(_config), assume_ip(_assume_ip), template_packet(build_pkt_template()),
      uniform_rand(_config.random_seed, 0, _config.total_flows - 1), zipf_rand(_config.random_seed, _config.zipf_param, _config.total_flows),
      pd(NULL), pdumper(NULL), client_dev_it(0), counters(0), flows_swapped(0), current_time(0), alarm_tick(0), next_alarm(-1) {
  for (device_t client_dev : config.client_devices) {
    warmup_writers.emplace(client_dev, PcapWriter(get_warmup_pcap_fname(nf, config, client_dev), assume_ip));
  }

  for (device_t dev : config.devices) {
    writers.emplace(dev, PcapWriter(get_pcap_fname(nf, config, dev), assume_ip));
  }

  for (flow_idx_t i = 0; i < config.total_flows; i++) {
    device_t client_dev = get_current_client_dev();
    advance_client_dev();

    flows_to_dev[i] = client_dev;
    counters[i]     = 0;
  }

  if (config.churn > 0) {
    alarm_tick = 60'000'000'000 / config.churn;
    next_alarm = alarm_tick;
  }

  reset_client_dev();
}

void TrafficGenerator::config_t::print() const {
  printf("----- Config -----\n");
  printf("out dir:     %s\n", out_dir.c_str());
  printf("#packets:    %lu\n", total_packets);
  printf("#flows:      %lu\n", total_flows);
  printf("packet size: %u\n", packet_size);
  printf("rate:        %lu Mbps\n", rate / 1'000'000);
  printf("churn:       %lu fpm\n", churn);
  switch (traffic_type) {
  case TrafficType::Uniform:
    printf("traffic:     uniform\n");
    break;
  case TrafficType::Zipf:
    printf("traffic:     zipf\n");
    printf("zipf param:  %f\n", zipf_param);
    break;
  }
  printf("devices: [ ");
  for (device_t dev : devices) {
    printf("%u ", dev);
  }
  printf("]\n");
  printf("clients: [ ");
  for (device_t dev : client_devices) {
    printf("%u ", dev);
  }
  printf("]\n");
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

  std::memset(pkt.payload, 0x42, sizeof(pkt.payload));

  return pkt;
}

void TrafficGenerator::generate() {
  const bytes_t hdrs_len = assume_ip ? get_hdrs_len() - sizeof(ether_hdr_t) : get_hdrs_len();
  const bytes_t pkt_len  = config.packet_size;
  const u64 goal         = config.total_packets;
  u64 counter            = 0;
  int progress           = -1;

  for (const auto &[dev, writer] : writers) {
    printf("Dev %u: %s\n", dev, writer.get_output_fname().c_str());
  }

  // FIXME: this is too slow! we should be sending at all ports at the same time.
  for (u64 i = 0; i < config.total_packets; i++) {
    if (next_alarm >= 0 && current_time >= next_alarm) {
      const device_t client_dev = get_current_client_dev();
      advance_client_dev();

      const flow_idx_t chosen_swap_flow_idx = uniform_rand.generate();
      random_swap_flow(chosen_swap_flow_idx);
      flows_swapped++;
      next_alarm += alarm_tick;

      flows_to_dev[chosen_swap_flow_idx] = client_dev;
      counters[chosen_swap_flow_idx]     = 0;
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

    const device_t dev = flows_to_dev.at(flow_idx);
    const pkt_t pkt    = build_packet(dev, flow_idx);
    const u8 *data     = assume_ip ? reinterpret_cast<const u8 *>(&pkt.ip_hdr) : reinterpret_cast<const u8 *>(&pkt);

    writers.at(dev).write(data, hdrs_len, pkt_len, current_time);

    if (std::optional<device_t> res_dev = get_response_dev(dev, flow_idx)) {
      flows_to_dev[flow_idx] = *res_dev;
    }

    tick();

    counters[flow_idx]++;
    counter++;
    const int current_progress = (counter * 100) / goal;

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
  const bytes_t hdrs_len = assume_ip ? get_hdrs_len() - sizeof(ether_hdr_t) : get_hdrs_len();
  const bytes_t pkt_len  = config.packet_size;

  u64 counter  = 0;
  u64 goal     = config.total_flows;
  int progress = -1;

  for (const auto &[dev, warmup_writer] : warmup_writers) {
    printf("Warmup dev %u: %s\n", dev, warmup_writer.get_output_fname().c_str());
  }

  for (auto &[target_dev, warmup_writer] : warmup_writers) {
    for (const auto &[flow_idx, dev] : flows_to_dev) {
      if (dev != target_dev) {
        continue;
      }

      const pkt_t pkt = build_packet(dev, flow_idx);
      const u8 *data  = assume_ip ? reinterpret_cast<const u8 *>(&pkt.ip_hdr) : reinterpret_cast<const u8 *>(&pkt);

      warmup_writer.write(data, hdrs_len, pkt_len, current_time);

      counter++;
      const int current_progress = (counter * 100) / goal;

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