#include "util.hpp"

#include <unordered_set>
#include <filesystem>
#include <optional>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s [pcap]\n", argv[0]);
    return 1;
  }

  const std::filesystem::path pcap_file = std::string(argv[1]);

  if (!std::filesystem::exists(pcap_file)) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    exit(1);
  }

  PcapReader pcap_reader(argv[1]);

  std::filesystem::path lan_pcap_file = pcap_file.filename().stem();
  std::filesystem::path wan_pcap_file = pcap_file.filename().stem();

  lan_pcap_file += "-lan.pcap";
  wan_pcap_file += "-wan.pcap";

  PcapWriter lan_writer(lan_pcap_file.c_str(), pcap_reader.assumes_ip());
  PcapWriter wan_writer(wan_pcap_file.c_str(), pcap_reader.assumes_ip());

  std::unordered_set<flow_t, flow_t::flow_hash_t> lan_flows;
  std::unordered_set<flow_t, flow_t::flow_hash_t> wan_flows;

  uint64_t total_pkts = pcap_reader.get_total_pkts();
  uint64_t pkt_count = 0;
  int progress = -1;

  const u_char *pkt;
  uint16_t sz;
  time_ns_t ts;
  std::optional<flow_t> flow;

  while (pcap_reader.read(pkt, sz, ts, flow)) {
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
      wan_writer.write(pkt, sz, ts);
      continue;
    }

    if (lan_flows.find(flow.value()) == lan_flows.end()) {
      flow_t res(flow->dst_ip, flow->src_ip, flow->dst_port, flow->src_port);
      lan_flows.emplace(flow.value());
      wan_flows.emplace(res);
    }

    lan_writer.write(pkt, sz, ts);
  }

  printf("\n");

  return 0;
}