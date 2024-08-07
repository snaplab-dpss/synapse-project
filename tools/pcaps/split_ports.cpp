#include <arpa/inet.h>
#include <assert.h>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pcap.h>
#include <stdint.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <optional>

#include "util.hpp"

struct pkt_hdr_t {
  ether_header eth_hdr;
  iphdr ip_hdr;
  udphdr udp_hdr;
} __attribute__((packed));

struct flow_t {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;

  flow_t() : src_ip(0), dst_ip(0), src_port(0), dst_port(0) {}

  flow_t(const flow_t &flow)
      : src_ip(flow.src_ip), dst_ip(flow.dst_ip), src_port(flow.src_port),
        dst_port(flow.dst_port) {}

  flow_t(uint32_t _src_ip, uint32_t _dst_ip, uint16_t _src_port,
         uint16_t _dst_port)
      : src_ip(_src_ip), dst_ip(_dst_ip), src_port(_src_port),
        dst_port(_dst_port) {}

  bool operator==(const flow_t &other) const {
    return other.src_ip == src_ip && dst_ip == other.dst_ip &&
           src_port == other.src_port && dst_port == other.dst_port;
  }

  struct flow_hash_t {
    std::size_t operator()(const flow_t &flow) const {
      return std::hash<in_addr_t>()(flow.src_ip) ^
             std::hash<in_addr_t>()(flow.dst_ip) ^
             std::hash<uint16_t>()(flow.src_port) ^
             std::hash<uint16_t>()(flow.dst_port);
    }
  };
};

class PcapReader {
private:
  pcap_t *pd;
  bool assume_ip;
  long pcap_start;
  uint64_t total_pkts;

public:
  PcapReader(const std::string &input_fname) : assume_ip(false), total_pkts(0) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pd = pcap_open_offline(input_fname.c_str(), errbuf);

    if (pd == nullptr) {
      fprintf(stderr, "Unable to open file %s: %s\n", input_fname.c_str(),
              errbuf);
      exit(1);
    }

    int link_hdr_type = pcap_datalink(pd);

    switch (link_hdr_type) {
    case DLT_EN10MB:
      // Normal ethernet, as expected. Nothing to do here.
      break;
    case DLT_RAW:
      // Contains raw IP packets.
      assume_ip = true;
      break;
    default: {
      fprintf(stderr, "Unknown header type (%d)", link_hdr_type);
      exit(1);
    }
    }

    FILE *pcap_fptr = pcap_file(pd);
    assert(pcap_fptr && "Invalid pcap file pointer");
    pcap_start = ftell(pcap_fptr);

    count_total_pkts();
  }

  bool assumes_ip() const { return assume_ip; }
  uint64_t get_total_pkts() const { return total_pkts; }

  bool read(const u_char *&pkt, uint16_t &sz, time_ns_t &ts,
            std::optional<flow_t> &flow) {
    const u_char *data;
    struct pcap_pkthdr *header;

    if (pcap_next_ex(pd, &header, &data) != 1) {
      return false;
    }

    pkt = data;
    sz = header->caplen;
    ts = header->ts.tv_sec * 1'000'000'000 + header->ts.tv_usec * 1'000;

    if (!assume_ip) {
      const ether_header *ether_hdr =
          reinterpret_cast<const ether_header *>(data);
      data += sizeof(ether_header);

      uint16_t ether_type = ntohs(ether_hdr->ether_type);

      if (ether_type != ETHERTYPE_IP) {
        return true;
      }
    }

    const ip *ip_hdr = reinterpret_cast<const ip *>(data);
    data += sizeof(ip);

    if (ip_hdr->ip_v != 4) {
      return true;
    }

    uint16_t size_hint = ntohs(ip_hdr->ip_len) + sizeof(ether_header);

    uint32_t src = ntohl(ip_hdr->ip_src.s_addr);
    uint32_t dst = ntohl(ip_hdr->ip_dst.s_addr);

    uint16_t sport;
    uint16_t dport;

    // We only support TCP/UDP
    switch (ip_hdr->ip_p) {
    case IPPROTO_TCP: {
      const tcphdr *tcp_hdr = reinterpret_cast<const tcphdr *>(data);
      sport = ntohs(tcp_hdr->th_sport);
      dport = ntohs(tcp_hdr->th_dport);
    } break;

    case IPPROTO_UDP: {
      const udphdr *udp_hdr = reinterpret_cast<const udphdr *>(data);
      sport = ntohs(udp_hdr->uh_sport);
      dport = ntohs(udp_hdr->uh_dport);
    } break;
    default: {
      return true;
    }
    }

    flow = flow_t(src, dst, sport, dport);

    return true;
  }

  // WARNING: this does not work on windows!
  // https://winpcap-users.winpcap.narkive.com/scCKD3x2/packet-random-access-using-file-seek
  void rewind() {
    FILE *pcap_fptr = pcap_file(pd);
    fseek(pcap_fptr, pcap_start, SEEK_SET);
  }

private:
  void count_total_pkts() {
    total_pkts = 0;

    const u_char *pkt;
    uint16_t sz;
    time_ns_t ts;
    std::optional<flow_t> flow;

    while (read(pkt, sz, ts, flow)) {
      total_pkts++;
    }

    rewind();
  }
};

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