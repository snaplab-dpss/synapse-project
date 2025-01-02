#include <unordered_set>
#include <filesystem>
#include <optional>

#include <CLI/CLI.hpp>

#include "../src/pcap.h"

using namespace synapse;

struct pkt_hdr_t {
  ipv4_hdr_t ip_hdr;
  udp_hdr_t udp_hdr;
} __attribute__((packed));

pkt_hdr_t build_pkt(const flow_t &flow, uint16_t len) {
  pkt_hdr_t pkt;

  pkt.ip_hdr.version = 4;
  pkt.ip_hdr.ihl = 5;
  pkt.ip_hdr.type_of_service = 0;
  pkt.ip_hdr.total_length = htons(sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t));
  pkt.ip_hdr.packet_id = 0;
  pkt.ip_hdr.fragment_offset = 0;
  pkt.ip_hdr.time_to_live = 64;
  pkt.ip_hdr.next_proto_id = IPPROTO_UDP;
  pkt.ip_hdr.hdr_checksum = 0;
  pkt.ip_hdr.src_addr = 0;
  pkt.ip_hdr.dst_addr = 0;

  pkt.udp_hdr.src_port = 0;
  pkt.udp_hdr.dst_port = 0;
  pkt.udp_hdr.len = htons(sizeof(udp_hdr_t));
  pkt.udp_hdr.checksum = 0;

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

    // Subtract 4 bytes for FCS.
    u16 len = sz - 4u;

    pkt_hdr_t pkt = build_pkt(flow.value(), len);
    u16 caplen = sizeof(pkt_hdr_t);

    filtered_writer.write((const u_char *)&pkt, caplen, len, ts);
  }

  printf("\n");

  return 0;
}