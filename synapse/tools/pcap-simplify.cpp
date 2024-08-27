#include <unordered_set>
#include <filesystem>
#include <optional>

#include <CLI/CLI.hpp>

#include "../src/pcap.h"

struct pkt_hdr_t {
  iphdr ip_hdr;
  udphdr udp_hdr;
} __attribute__((packed));

pkt_hdr_t build_pkt(const flow_t &flow, uint16_t len) {
  pkt_hdr_t pkt;

  pkt.ip_hdr.version = 4;
  pkt.ip_hdr.ihl = 5;
  pkt.ip_hdr.tos = 0;
  pkt.ip_hdr.id = 0;
  pkt.ip_hdr.frag_off = 0;
  pkt.ip_hdr.ttl = 64;
  pkt.ip_hdr.protocol = IPPROTO_UDP;
  pkt.ip_hdr.check = 0;
  pkt.ip_hdr.saddr = htonl(flow.src_ip);
  pkt.ip_hdr.daddr = htonl(flow.dst_ip);
  pkt.ip_hdr.tot_len = htons(len - sizeof(ether_header));

  pkt.udp_hdr.source = htons(flow.src_port);
  pkt.udp_hdr.dest = htons(flow.dst_port);
  pkt.udp_hdr.len = htons(len - sizeof(ether_header) - sizeof(iphdr));
  pkt.udp_hdr.check = 0;

  return pkt;
}

int main(int argc, char *argv[]) {
  CLI::App app{"Simplify/filter pcap traffic into UDP flows."};

  std::filesystem::path pcap_file;

  app.add_option("pcap", pcap_file, "Pcap file.")->required();

  CLI11_PARSE(app, argc, argv);

  if (!std::filesystem::exists(pcap_file)) {
    fprintf(stderr, "File %s not found\n", pcap_file.c_str());
    exit(1);
  }

  PcapReader pcap_reader(pcap_file);

  std::filesystem::path filtered_pcap_file = pcap_file.filename().stem();
  filtered_pcap_file += "-filtered.pcap";

  PcapWriter filtered_writer(filtered_pcap_file.c_str(), true, true);

  uint64_t total_pkts = pcap_reader.get_total_pkts();
  uint64_t pkt_count = 0;
  int progress = -1;

  const u_char *pkt;
  uint16_t hdrs_len;
  uint16_t sz;
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

    // Subtract 4 bytes for FCS.
    uint16_t len = sz - 4u;

    pkt_hdr_t pkt = build_pkt(flow.value(), len);
    uint16_t caplen = sizeof(pkt_hdr_t);

    filtered_writer.write((const u_char *)&pkt, caplen, len, ts);
  }

  printf("\n");

  return 0;
}