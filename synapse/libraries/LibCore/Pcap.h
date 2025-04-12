#pragma once

#include <LibCore/Types.h>
#include <LibCore/Net.h>
#include <LibCore/Debug.h>

#include <byteswap.h>
#include <pcap.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <functional>
#include <random>
#include <chrono>
#include <ctime>
#include <iostream>
#include <optional>

namespace LibCore {

struct flow_t {
  in_addr_t src_ip;
  in_addr_t dst_ip;
  in_port_t src_port;
  in_port_t dst_port;

  flow_t() : src_ip(0), dst_ip(0), src_port(0), dst_port(0) {}

  flow_t(const flow_t &flow) : src_ip(flow.src_ip), dst_ip(flow.dst_ip), src_port(flow.src_port), dst_port(flow.dst_port) {}

  flow_t(in_addr_t _src_ip, in_addr_t _dst_ip, in_port_t _src_port, in_port_t _dst_port)
      : src_ip(_src_ip), dst_ip(_dst_ip), src_port(_src_port), dst_port(_dst_port) {}

  flow_t &operator=(const flow_t &flow) = default;

  flow_t invert() const { return flow_t(dst_ip, src_ip, dst_port, src_port); }

  bool operator==(const flow_t &other) const {
    return src_ip == other.src_ip && dst_ip == other.dst_ip && src_port == other.src_port && dst_port == other.dst_port;
  }

  struct flow_hash_t {
    std::size_t operator()(const flow_t &flow) const {
      return std::hash<in_addr_t>()(flow.src_ip) ^ std::hash<in_addr_t>()(flow.dst_ip) ^ std::hash<in_port_t>()(flow.src_port) ^
             std::hash<in_port_t>()(flow.dst_port);
    }
  };
};

inline std::ostream &operator<<(std::ostream &os, const flow_t &flow) {
  os << "{";

  os << LibCore::ipv4_to_str(flow.src_ip);
  os << ":";
  os << bswap16(flow.src_port);

  os << " -> ";

  os << LibCore::ipv4_to_str(flow.dst_ip);
  os << ":";
  os << bswap16(flow.dst_port);

  os << "}";
  return os;
}

// Symmetric
struct sflow_t {
  in_addr_t src_ip;
  in_addr_t dst_ip;
  in_port_t src_port;
  in_port_t dst_port;

  sflow_t() : src_ip(0), dst_ip(0), src_port(0), dst_port(0) {}

  sflow_t(const flow_t &flow) : src_ip(flow.src_ip), dst_ip(flow.dst_ip), src_port(flow.src_port), dst_port(flow.dst_port) {}

  sflow_t(const sflow_t &flow) : src_ip(flow.src_ip), dst_ip(flow.dst_ip), src_port(flow.src_port), dst_port(flow.dst_port) {}

  sflow_t(in_addr_t _src_ip, in_addr_t _dst_ip, in_port_t _src_port, in_port_t _dst_port)
      : src_ip(_src_ip), dst_ip(_dst_ip), src_port(_src_port), dst_port(_dst_port) {}

  bool operator==(const sflow_t &other) const {
    return (src_ip == other.src_ip && dst_ip == other.dst_ip && src_port == other.src_port && dst_port == other.dst_port) ||
           (src_ip == other.dst_ip && dst_ip == other.src_ip && src_port == other.dst_port && dst_port == other.src_port);
  }

  struct flow_hash_t {
    std::size_t operator()(const sflow_t &flow) const {
      return std::hash<in_addr_t>()(flow.src_ip) ^ std::hash<in_addr_t>()(flow.dst_ip) ^ std::hash<in_port_t>()(flow.src_port) ^
             std::hash<in_port_t>()(flow.dst_port);
    }
  };
};

inline std::string fmt(u64 n) {
  std::stringstream ss;
  int rem = n % 1000;
  n /= 1000;

  if (n > 0) {
    ss << fmt(n);
    ss << "," << std::setfill('0') << std::setw(3) << rem;
  } else {
    ss << rem;
  }

  return ss.str();
}

inline std::string fmt(double d) {
  std::stringstream ss;

  u64 n = static_cast<u64>(d);

  ss << fmt(n);
  ss << ".";
  ss << std::setfill('0') << std::setw(2) << static_cast<u64>((d - n) * 100);

  return ss.str();
}

inline std::string fmt_time_hh(time_ns_t ns) {
  std::stringstream ss;

  time_s_t seconds       = ns / BILLION;
  time_us_t microseconds = (ns % BILLION) / THOUSAND;

  std::chrono::seconds sec(seconds);
  std::chrono::microseconds microsec(microseconds);

  std::chrono::system_clock::time_point time_point = std::chrono::system_clock::time_point(sec) + microsec;

  std::time_t time = std::chrono::system_clock::to_time_t(time_point);
  std::tm utc_tm   = *std::gmtime(&time);

  ss << std::put_time(&utc_tm, "%Y-%m-%d %H:%M:%S") << "." << std::setw(6) << std::setfill('0') << microseconds << " UTC";

  return ss.str();
}

inline std::string fmt_time_duration_hh(time_ns_t start, time_ns_t end) {
  std::stringstream ss;

  time_s_t start_seconds       = start / BILLION;
  time_us_t start_microseconds = (start % BILLION) / THOUSAND;

  time_s_t end_seconds       = end / BILLION;
  time_us_t end_microseconds = (end % BILLION) / THOUSAND;

  std::chrono::system_clock::time_point start_time =
      std::chrono::system_clock::time_point(std::chrono::seconds(start_seconds)) + std::chrono::microseconds(start_microseconds);

  std::chrono::system_clock::time_point end_time =
      std::chrono::system_clock::time_point(std::chrono::seconds(end_seconds)) + std::chrono::microseconds(end_microseconds);

  auto duration = end_time - start_time;

  // Break down the duration into hours, minutes, seconds, and microseconds
  auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
  duration -= hours;
  auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
  duration -= minutes;
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
  duration -= seconds;
  auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);

  if (hours.count() > 0) {
    ss << hours.count() << " hours ";
  }

  if (minutes.count() > 0) {
    ss << minutes.count() << " min ";
  }

  ss << seconds.count() + (static_cast<double>(microseconds.count()) / MILLION) << " sec ";

  return ss.str();
}

/* Compute checksum for count bytes starting at addr, using one's complement of
 * one's complement sum*/
static u16 compute_checksum(u8 *addr, u32 count) {
  unsigned long sum = 0;
  while (count > 1) {
    sum += *addr++;
    count -= 2;
  }
  // if any bytes left, pad the bytes and add
  if (count > 0) {
    sum += ((*addr) & htons(0xFF00));
  }
  // Fold sum to 16 bits: add carrier to result
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  // one's complement
  sum = ~sum;
  return ((u16)sum);
}

/* set ip checksum of a given ip header*/
inline void compute_ip_checksum(ipv4_hdr_t *ip_hdr) {
  ip_hdr->hdr_checksum = 0;
  ip_hdr->hdr_checksum = compute_checksum((u8 *)ip_hdr, ip_hdr->ihl << 2);
}

/* set tcp checksum: given IP header and UDP datagram */
inline void compute_udp_checksum(ipv4_hdr_t *ip_hdr, u16 *ipPayload) {
  unsigned long sum  = 0;
  udp_hdr_t *udp_hdr = (udp_hdr_t *)(ipPayload);
  u16 udpLen         = htons(udp_hdr->len);
  // printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~udp len=%dn", udpLen);
  // add the pseudo header
  // printf("add pseudo headern");
  // the source ip
  sum += (ip_hdr->src_addr >> 16) & 0xFFFF;
  sum += (ip_hdr->src_addr) & 0xFFFF;
  // the dest ip
  sum += (ip_hdr->dst_addr >> 16) & 0xFFFF;
  sum += (ip_hdr->dst_addr) & 0xFFFF;
  // protocol and reserved: 17
  sum += htons(IPPROTO_UDP);
  // the length
  sum += udp_hdr->len;

  // add the IP payload
  // printf("add ip payloadn");
  // initialize checksum to 0
  udp_hdr->checksum = 0;
  while (udpLen > 1) {
    sum += *ipPayload++;
    udpLen -= 2;
  }
  // if any bytes left, pad the bytes and add
  if (udpLen > 0) {
    // printf("+++++++++++++++padding: %dn", udpLen);
    sum += ((*ipPayload) & htons(0xFF00));
  }
  // Fold sum to 16 bits: add carrier to result
  // printf("add carriern");
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  // printf("one's complementn");
  sum = ~sum;
  // set computation result
  udp_hdr->checksum = ((u16)sum == 0x0000) ? 0xFFFF : (u16)sum;
}

inline bool parse_etheraddr(const char *str, struct ether_addr_t *addr) {
  return sscanf(str, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", addr->addr_bytes + 0, addr->addr_bytes + 1, addr->addr_bytes + 2,
                addr->addr_bytes + 3, addr->addr_bytes + 4, addr->addr_bytes + 5) == 6;
}

inline bool parse_ipv4addr(const char *str, u32 *addr) {
  u8 a, b, c, d;
  if (sscanf(str, "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d) == 4) {
    *addr = ((u32)a << 0) | ((u32)b << 8) | ((u32)c << 16) | ((u32)d << 24);
    return true;
  }
  return false;
}

inline uintmax_t parse_int(const char *str, const char *name, int base, char next) {
  char *temp;
  intmax_t result = strtoimax(str, &temp, base);

  // There's also a weird failure case with overflows, but let's not care
  if (temp == str || *temp != next) {
    panic("Error while parsing '%s': %s", name, str);
  }

  return result;
}

class PcapReader {
private:
  pcap_t *pd;
  bool assume_ip;
  long pcap_start;
  u64 total_pkts;
  time_ns_t start;
  time_ns_t end;

public:
  PcapReader(const std::string &input_fname) : assume_ip(false), total_pkts(0) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pd = pcap_open_offline(input_fname.c_str(), errbuf);

    if (pd == nullptr) {
      panic("Unable to open file %s: %s", input_fname.c_str(), errbuf);
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
      panic("Unknown header type (%d)", link_hdr_type);
    }
    }

    FILE *pcap_fptr = pcap_file(pd);
    assert(pcap_fptr && "Invalid pcap file pointer");
    pcap_start = ftell(pcap_fptr);

    run_preamble();
  }

  bool assumes_ip() const { return assume_ip; }
  u64 get_total_pkts() const { return total_pkts; }
  time_ns_t get_start() const { return start; }
  time_ns_t get_end() const { return end; }

  bool read(const u8 *&pkt, u16 &hdrs_len, u16 &total_len, time_ns_t &ts, std::optional<flow_t> &flow) {
    const u8 *data;
    struct pcap_pkthdr *header;

    if (pcap_next_ex(pd, &header, &data) != 1) {
      return false;
    }

    pkt       = data;
    hdrs_len  = 0;
    total_len = header->len + CRC_SIZE_BYTES;
    ts        = header->ts.tv_sec * 1'000'000'000 + header->ts.tv_usec * 1'000;

    if (assume_ip) {
      total_len += sizeof(ether_hdr_t);
    } else {
      const ether_hdr_t *ether_hdr = reinterpret_cast<const ether_hdr_t *>(data);
      data += sizeof(ether_hdr_t);
      hdrs_len += sizeof(ether_hdr_t);

      u16 ether_type = ntohs(ether_hdr->ether_type);

      if (ether_type == ETHERTYPE_VLAN) {
        // The VLAN header starts at the Ethernet ethertype field,
        // so we need to rollback.
        data = reinterpret_cast<const u8 *>(&ether_hdr->ether_type);

        // Ignore the VLAN header and advance the data pointer.
        data += sizeof(vlan_hdr_t);

        // Grab the encapsulated ethertype and offset the data pointer.
        ether_type = ntohs(reinterpret_cast<const u16 *>(data)[0]);
        data += sizeof(u16);
        hdrs_len += sizeof(vlan_hdr_t) + sizeof(u16);
      }

      if (ether_type != ETHERTYPE_IP) {
        hdrs_len = total_len;
        return true;
      }
    }

    const ipv4_hdr_t *ip_hdr = reinterpret_cast<const ipv4_hdr_t *>(data);
    data += sizeof(ipv4_hdr_t);
    hdrs_len += sizeof(ipv4_hdr_t);

    if (ip_hdr->version != 4) {
      return true;
    }

    u32 src = ntohl(ip_hdr->src_addr);
    u32 dst = ntohl(ip_hdr->dst_addr);

    u16 sport = 0;
    u16 dport = 0;

    // We only support TCP/UDP
    switch (ip_hdr->next_proto_id) {
    case IPPROTO_TCP: {
      const tcp_hdr_t *tcp_hdr = reinterpret_cast<const tcp_hdr_t *>(data);
      hdrs_len += sizeof(tcp_hdr_t);
      sport = ntohs(tcp_hdr->src_port);
      dport = ntohs(tcp_hdr->dst_port);
    } break;

    case IPPROTO_UDP: {
      const udp_hdr_t *udp_hdr = reinterpret_cast<const udp_hdr_t *>(data);
      hdrs_len += sizeof(udp_hdr_t);
      sport = ntohs(udp_hdr->src_port);
      dport = ntohs(udp_hdr->dst_port);
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
  void run_preamble() {
    total_pkts = 0;

    const u8 *pkt;
    u16 hdrs_len;
    u16 sz;
    time_ns_t ts;
    std::optional<flow_t> flow;
    bool set_start = false;

    while (read(pkt, hdrs_len, sz, ts, flow)) {
      total_pkts++;

      if (!set_start) {
        start     = ts;
        set_start = true;
      }

      end = ts;
    }

    rewind();
  }
};

class PcapWriter {
private:
  std::filesystem::path output_fname;
  pcap_t *pd;
  pcap_dumper_t *pdumper;
  bool assume_ip;
  bool compact;

public:
  PcapWriter(const std::filesystem::path &_output_fname, bool _assume_ip, bool _compact)
      : output_fname(_output_fname), pd(NULL), pdumper(NULL), assume_ip(_assume_ip), compact(_compact) {
    if (assume_ip) {
      pd = pcap_open_dead(DLT_RAW, 65535 /* snaplen */);
    } else {
      pd = pcap_open_dead(DLT_EN10MB, 65535 /* snaplen */);
    }

    pdumper = pcap_dump_open(pd, _output_fname.c_str());

    if (pdumper == nullptr) {
      panic("Unable to open file %s for writing", _output_fname.c_str());
    }
  }

  PcapWriter(const std::filesystem::path &_output_fname) : PcapWriter(_output_fname, false, true) {}

  const std::filesystem::path &get_output_fname() const { return output_fname; }

  void write(const u8 *pkt, u16 hdrs_len, u16 total_len, time_ns_t ts) {
    time_s_t sec   = ts / 1'000'000'000;
    time_us_t usec = (ts % 1'000'000'000) / 1'000;
    pcap_pkthdr pcap_hdr{{sec, usec}, hdrs_len, total_len};
    pcap_dump((u8 *)pdumper, &pcap_hdr, pkt);
  }

  ~PcapWriter() {
    if (pd)
      pcap_close(pd);
    if (pdumper)
      pcap_dump_close(pdumper);
  }
};

inline in_addr_t random_addr() {
  std::stringstream ss;
  ss << rand() % 256 << ".";
  ss << rand() % 256 << ".";
  ss << rand() % 256 << ".";
  ss << rand() % 256;
  return inet_addr(ss.str().c_str());
}

inline in_port_t random_port() { return rand() % 65536; }

} // namespace LibCore