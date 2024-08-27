#include <unordered_set>
#include <filesystem>
#include <optional>

#include <CLI/CLI.hpp>

#include "../src/pcap.h"

int main(int argc, char *argv[]) {
  CLI::App app{"Split pcap traffic into multiple ports."};

  std::filesystem::path pcap_file;

  app.add_option("pcap", pcap_file, "Pcap file.")->required();

  CLI11_PARSE(app, argc, argv);

  if (!std::filesystem::exists(pcap_file)) {
    fprintf(stderr, "File %s not found\n", pcap_file.c_str());
    exit(1);
  }

  PcapReader pcap_reader(pcap_file);

  std::filesystem::path lan_pcap_file = pcap_file.filename().stem();
  std::filesystem::path wan_pcap_file = pcap_file.filename().stem();

  lan_pcap_file += "-lan.pcap";
  wan_pcap_file += "-wan.pcap";

  PcapWriter lan_writer(lan_pcap_file.c_str(), pcap_reader.assumes_ip(), true);
  PcapWriter wan_writer(wan_pcap_file.c_str(), pcap_reader.assumes_ip(), true);

  std::unordered_set<flow_t, flow_t::flow_hash_t> lan_flows;
  std::unordered_set<flow_t, flow_t::flow_hash_t> wan_flows;

  u64 total_pkts = pcap_reader.get_total_pkts();
  u64 pkt_count = 0;
  int progress = -1;

  const u_char *pkt;
  u16 hdrs_len;
  u16 sz;
  time_ns_t ts;
  std::optional<flow_t> flow;

  while (pcap_reader.read(pkt, hdrs_len, sz, ts, flow)) {
    pkt_count++;

    int current_progress = 100.0 * pkt_count / total_pkts;

    if (current_progress > progress) {
      progress = current_progress;
      printf("\r[Progress %3d%%]", progress);
      fflush(stdout);
    }

    if (!flow.has_value()) {
      continue;
    }

    if (wan_flows.find(flow.value()) != wan_flows.end()) {
      wan_writer.write(pkt, hdrs_len, sz, ts);
      continue;
    }

    if (lan_flows.find(flow.value()) == lan_flows.end()) {
      flow_t res(flow->dst_ip, flow->src_ip, flow->dst_port, flow->src_port);
      lan_flows.emplace(flow.value());
      wan_flows.emplace(res);
    }

    lan_writer.write(pkt, hdrs_len, sz, ts);
  }

  printf("\n");

  return 0;
}