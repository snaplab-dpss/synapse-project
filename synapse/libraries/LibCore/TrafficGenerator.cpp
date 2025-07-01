#include <LibCore/TrafficGenerator.h>

namespace LibCore {

TrafficGenerator::TrafficGenerator(const std::string &_nf, const config_t &_config, bool _assume_ip)
    : nf(_nf), config(_config), assume_ip(_assume_ip), template_packet(build_pkt_template()), seeds_random_engine(config.random_seed, 0, UINT32_MAX),
      churn_random_engine(seeds_random_engine.generate(), 0, config.total_flows - 1),
      flows_random_engine_uniform(seeds_random_engine.generate(), 0, config.total_flows - 1),
      flows_random_engine_zipf(seeds_random_engine.generate(), config.zipf_param, 0, config.total_flows - 1), pd(NULL), pdumper(NULL),
      client_dev_it(0), counters(config.total_flows, 0), flows_swapped(0), current_time(0), alarm_tick(0), next_alarm(-1) {
  for (device_t client_dev : config.client_devices) {
    warmup_writers.emplace(client_dev, PcapWriter(get_warmup_pcap_fname(nf, config, client_dev), assume_ip));
    client_to_active_device.emplace(client_dev, client_dev);
  }

  for (device_t dev : config.devices) {
    writers.emplace(dev, PcapWriter(get_pcap_fname(nf, config, dev), assume_ip));
  }

  if (config.churn > 0) {
    alarm_tick = 60 * BILLION / config.churn;
    next_alarm = alarm_tick;
  }

  reset_client_dev();
}

void TrafficGenerator::config_t::print() const {
  printf("----- Config -----\n");
  printf("out dir:     %s\n", out_dir.c_str());
  printf("#packets:    %lu\n", total_packets);
  printf("#flows:      %lu\n", total_flows);
  printf("packet size: %u\n", packet_size_without_crc);
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
  pkt.ip_hdr.total_length    = htons(config.packet_size_without_crc - sizeof(pkt.eth_hdr));
  pkt.ip_hdr.packet_id       = 0;
  pkt.ip_hdr.fragment_offset = 0;
  pkt.ip_hdr.time_to_live    = 64;
  pkt.ip_hdr.next_proto_id   = IPPROTO_UDP;
  pkt.ip_hdr.hdr_checksum    = 0;
  pkt.ip_hdr.src_addr        = 0;
  pkt.ip_hdr.dst_addr        = 0;

  pkt.udp_hdr.src_port = 0;
  pkt.udp_hdr.dst_port = 0;
  pkt.udp_hdr.len      = htons(config.packet_size_without_crc - (sizeof(pkt.eth_hdr) + sizeof(pkt.ip_hdr)));
  pkt.udp_hdr.checksum = 0;

  std::memset(pkt.payload, 0x42, sizeof(pkt.payload));

  return pkt;
}

void TrafficGenerator::generate() {
  const bytes_t hdrs_len = assume_ip ? get_hdrs_len() - sizeof(ether_hdr_t) : get_hdrs_len();
  const bytes_t pkt_len  = assume_ip ? config.packet_size_without_crc - sizeof(ether_hdr_t) : config.packet_size_without_crc;
  const u64 goal         = config.total_packets;
  u64 counter            = 0;
  int progress           = -1;

  for (const auto &[dev, writer] : writers) {
    printf("Dev %u: %s\n", dev, writer.get_output_fname().c_str());
  }

  const device_t first_client_dev = get_current_client_dev();

  while (counter < config.total_packets) {
    const device_t client_dev = get_current_client_dev();
    const device_t dev        = client_to_active_device.at(client_dev);
    advance_client_dev();

    if (client_dev == first_client_dev) {
      tick();
    }

    if (next_alarm >= 0 && current_time >= next_alarm) {
      flow_idx_t chosen_swap_flow_idx = churn_random_engine.generate();
      random_swap_flow(chosen_swap_flow_idx);
      counters[chosen_swap_flow_idx] = 0;
      flows_swapped++;
      next_alarm += alarm_tick;
    }

    const flow_idx_t flow_idx      = get_next_flow_idx();
    const std::optional<pkt_t> pkt = build_packet(dev, flow_idx);

    if (std::optional<device_t> res_dev = get_response_dev(dev, flow_idx)) {
      client_to_active_device[client_dev] = *res_dev;
    }

    if (!pkt.has_value()) {
      continue;
    }

    const u8 *data = assume_ip ? reinterpret_cast<const u8 *>(&pkt->ip_hdr) : reinterpret_cast<const u8 *>(&pkt.value());
    writers.at(dev).write(data, hdrs_len, pkt_len, current_time);

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
  std::vector<u64> sorted_counters = counters;
  std::sort(sorted_counters.begin(), sorted_counters.end(), std::greater{});

  u64 hh         = 0;
  u64 hh_packets = 0;
  for (size_t i = 0; i < config.total_flows; i++) {
    hh++;
    hh_packets += sorted_counters[i];
    if (hh_packets >= config.total_packets * 0.8) {
      break;
    }
  }

  printf("\n");
  printf("Base flows: %ld\n", config.total_flows);
  printf("Total flows: %ld\n", config.total_flows);
  printf("Swapped flows: %ld\n", flows_swapped);
  printf("HH: %ld flows (%.2f%%) %.2f%% volume\n", hh, 100.0 * hh / config.total_flows, 100.0 * hh_packets / config.total_packets);
  printf("Top 10 flows:\n");
  for (size_t i = 0; i < config.total_flows; i++) {
    printf("  flow %ld: %ld\n", i, sorted_counters[i]);

    if (i >= 9) {
      break;
    }
  }
}

void TrafficGenerator::generate_warmup() {
  const bytes_t hdrs_len = assume_ip ? get_hdrs_len() - sizeof(ether_hdr_t) : get_hdrs_len();
  const bytes_t pkt_len  = assume_ip ? config.packet_size_without_crc - sizeof(ether_hdr_t) : config.packet_size_without_crc;

  const u64 goal = config.total_flows * warmup_writers.size();
  u64 counter    = 0;
  int progress   = -1;

  for (const auto &[dev, warmup_writer] : warmup_writers) {
    printf("Warmup dev %u: %s\n", dev, warmup_writer.get_output_fname().c_str());
  }

  std::vector<flow_idx_t> flow_indices(config.total_flows);
  std::iota(flow_indices.begin(), flow_indices.end(), 0);

  for (auto &[target_dev, warmup_writer] : warmup_writers) {
    std::shuffle(flow_indices.begin(), flow_indices.end(), flows_random_engine_uniform.get_engine());

    for (flow_idx_t flow_idx : flow_indices) {
      const pkt_t pkt = build_warmup_packet(target_dev, flow_idx);
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