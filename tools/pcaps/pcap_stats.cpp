#include "util.hpp"

#include <unordered_set>
#include <unordered_map>
#include <map>
#include <filesystem>
#include <optional>
#include <time.h>

struct pkt_hdr_t {
  ether_header eth_hdr;
  iphdr ip_hdr;
  udphdr udp_hdr;
} __attribute__((packed));

struct pkt_data_t {
  std::unordered_set<flow_t, flow_t::flow_hash_t> unique_flows;
  std::unordered_set<uint32_t> unique_src_ips;
  std::unordered_set<uint32_t> unique_dst_ips;
  std::unordered_map<flow_t, unsigned, flow_t::flow_hash_t> flow_counter;
  std::vector<flow_t> flows;
  std::vector<uint16_t> pkt_sizes;

  uint64_t total_sz;
  uint64_t n_pkts;
  time_ns_t time;

  pkt_data_t() : total_sz(0), n_pkts(0), time(0) {}

  void add(const pkt_hdr_t *pkt_hdr, uint16_t sz) {
    auto flow = flow_t(pkt_hdr->ip_hdr.saddr, pkt_hdr->ip_hdr.daddr,
                       pkt_hdr->udp_hdr.uh_sport, pkt_hdr->udp_hdr.uh_dport);
    if (unique_flows.count(flow) == 0) {
      unique_flows.insert(flow);
    }

    if (unique_src_ips.count(flow.src_ip) == 0) {
      unique_src_ips.insert(flow.src_ip);
    }

    if (unique_dst_ips.count(flow.dst_ip) == 0) {
      unique_dst_ips.insert(flow.dst_ip);
    }

    flows.push_back(flow);
    flow_counter[flow]++;
    pkt_sizes.push_back(sz);
    total_sz += sz;
    n_pkts++;
    time +=
        (time_ns_t)(byte_to_bit(sz) / (double)(MAX_THROUGHPUT_GIGABIT_PER_SEC));
  }

  std::vector<flow_t> get_elephants() const {
    std::vector<std::pair<flow_t, unsigned>> flow_instance_counter;

    flow_instance_counter.assign(flow_counter.begin(), flow_counter.end());

    std::sort(
        flow_instance_counter.begin(), flow_instance_counter.end(),
        [](std::pair<flow_t, unsigned> lhs, std::pair<flow_t, unsigned> rhs) {
          return lhs.second > rhs.second;
        });

    std::vector<flow_t> elephants;
    auto counter = 0;
    for (auto flow_instances : flow_instance_counter) {
      auto flow = flow_instances.first;
      auto instances = flow_instances.second;

      elephants.push_back(flow);
      counter += instances;

      auto ratio = (double)counter / (double)n_pkts;

      if (ratio >= 0.8) {
        break;
      }
    }

    return elephants;
  }
};

uint16_t avg(const std::vector<uint16_t> &values) {
  uint64_t res = 0;
  for (const auto &v : values) {
    res += v;
  }
  return (uint16_t)(res / values.size());
}

uint16_t std_dev(const std::vector<uint16_t> &values, uint16_t avg) {
  uint64_t res = 0;
  for (const auto &v : values) {
    res += (v - avg) * (v - avg);
  }
  return (uint16_t)(sqrt((double)res / values.size()));
}

void dump_sizes(const std::vector<uint16_t> &sizes) {
  std::unordered_map<uint16_t, uint64_t> sizes_counter;

  for (auto s : sizes) {
    sizes_counter[s]++;
  }

  //   std::ofstream out;
  //   out.open(SIZE_DIST_DUMP, std::ios::out);
  //   for (auto size_counter = sizes_counter.begin();
  //        size_counter != sizes_counter.end(); size_counter++) {
  //     auto size = size_counter->first;
  //     auto counter = size_counter->second;

  //     out << size << "\t" << counter << "\n";
  //   }
  //   out.close();
}

void get_stats(const pkt_data_t &pkt_data) {
  auto pkt_size_avg = avg(pkt_data.pkt_sizes);
  auto pkt_size_std_dev = std_dev(pkt_data.pkt_sizes, pkt_size_avg);
  auto elephants = pkt_data.get_elephants();

  printf("Packets      %s\n", fmt(pkt_data.flows.size()).c_str());
  printf("Flows        %s\n", fmt(pkt_data.unique_flows.size()).c_str());
  printf("Sources      %s\n", fmt(pkt_data.unique_src_ips.size()).c_str());
  printf("Destinations %s\n", fmt(pkt_data.unique_dst_ips.size()).c_str());
  printf("Elephants    %s (%.2lf%% flows => >80%% traffic)\n",
         fmt(elephants.size()).c_str(),
         100.0 * elephants.size() / pkt_data.unique_flows.size());
  printf("Size         %u +- %u B\n", pkt_size_avg, pkt_size_std_dev);
  printf("Replay time  %s ns\n", fmt(pkt_data.time).c_str());

  dump_sizes(pkt_data.pkt_sizes);
}

class PDF {
private:
  std::vector<uint64_t> values;
  std::vector<uint64_t> counters;
  uint64_t total = 0;

public:
  PDF(uint64_t min, uint64_t max, uint64_t step) {
    assert(min <= max);

    for (uint64_t v = min; v <= max; v += step) {
      values.push_back(v);
      counters.push_back(0);
    }

    if ((max - min) % step != 0) {
      values.push_back(max);
      counters.push_back(0);
    }
  }

  void add(uint64_t value) {
    total++;
    for (size_t i = 0; i < values.size(); i++) {
      if (value <= values[i]) {
        counters[i]++;
        return;
      }
    }
  }

  std::map<uint64_t, float> get_pdf() const {
    std::map<uint64_t, float> cdf;
    for (size_t i = 0; i < values.size(); i++) {
      if (total == 0) {
        cdf[values[i]] = 0;
        continue;
      }
      cdf[values[i]] = (float)counters[i] / total;
    }
    return cdf;
  }
};

class CDF {
private:
  std::map<uint64_t, uint64_t> values;
  uint64_t total;

public:
  CDF() : total(0) {}

  void add(uint64_t value) {
    values[value]++;
    total++;
  }

  std::map<uint64_t, float> get_cdf() const {
    std::map<uint64_t, float> cdf;
    uint64_t accounted = 0;

    float next_p = 0;
    float step = 0.1;

    for (const auto &[value, count] : values) {
      accounted += count;

      if (accounted == total) {
        cdf[value] = 1;
        break;
      }

      float p = (float)accounted / total;

      if (p >= next_p) {
        cdf[value] = p;

        while (p >= next_p) {
          next_p += step;
        }
      }
    }

    return cdf;
  }

  double get_avg() const {
    double avg = 0;
    for (const auto &[value, count] : values) {
      avg += value * count;
    }
    return avg / total;
  }

  double get_stdev() const {
    double avg = get_avg();
    double stdev = 0;
    for (const auto &[value, count] : values) {
      stdev += (value - avg) * (value - avg) * count;
    }
    return sqrt(stdev / total);
  }
};

struct flow_ts {
  time_ns_t first;
  time_ns_t last;
  std::vector<time_ns_t> dts;
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

  std::unordered_set<flow_t, flow_t::flow_hash_t> flows;
  std::unordered_set<sflow_t, sflow_t::flow_hash_t> symm_flows;

  std::unordered_set<flow_t, flow_t::flow_hash_t> expected_responses;
  std::unordered_set<sflow_t, sflow_t::flow_hash_t> flows_with_responses;

  std::unordered_map<sflow_t, uint64_t, sflow_t::flow_hash_t> pkts_per_flow;
  std::unordered_map<sflow_t, flow_ts, sflow_t::flow_hash_t> flow_times;
  CDF pkt_sizes_cdf;

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

    pkt_sizes_cdf.add(sz);

    if (!flow.has_value()) {
      continue;
    }

    flows.insert(flow.value());
    symm_flows.insert(flow.value());
    pkts_per_flow[flow.value()]++;

    if (flows_with_responses.find(flow.value()) == flows_with_responses.end()) {
      auto expected_responses_it = expected_responses.find(flow.value());

      if (expected_responses_it != expected_responses.end()) {
        flows_with_responses.insert(flow.value());
        expected_responses.erase(expected_responses_it);
      } else {
        expected_responses.insert(flow.value().invert());
      }
    }

    auto flow_times_it = flow_times.find(flow.value());

    if (flow_times_it == flow_times.end()) {
      flow_times[flow.value()] = {
          .first = ts,
          .last = ts,
          .dts = {},
      };
    } else {
      flow_ts &fts = flow_times_it->second;
      time_ns_t dt = ts - fts.last;
      fts.last = ts;
      fts.dts.push_back(dt);
    }
  }

  CDF pkts_per_flow_cdf;
  for (const auto &[flow, pkts] : pkts_per_flow) {
    pkts_per_flow_cdf.add(pkts);
  }

  CDF flow_duration_us_cdf;
  CDF flow_dts_us_cdf;
  for (const auto &[flow, ts] : flow_times) {
    flow_duration_us_cdf.add((ts.last - ts.first) / THOUSAND);

    if (ts.dts.empty()) {
      continue;
    }

    time_us_t dt_sum = 0;
    for (const auto &dt : ts.dts) {
      dt_sum += dt / THOUSAND;
    }
    flow_dts_us_cdf.add(dt_sum / (float)ts.dts.size());
  }

  printf("\n");

  printf("\n");
  printf("###################### PCAP Stats ######################\n");
  printf("Start:                    %s\n",
         fmt_time_hh(pcap_reader.get_start()).c_str());
  printf("End:                      %s\n",
         fmt_time_hh(pcap_reader.get_end()).c_str());
  printf("Duration:                 %s\n",
         fmt_time_duration_hh(pcap_reader.get_start(), pcap_reader.get_end())
             .c_str());
  printf("Total packets:            %s\n", fmt(pkt_count).c_str());
  printf("Pkt sizes:                %.2f ± %.2f B\n", pkt_sizes_cdf.get_avg(),
         pkt_sizes_cdf.get_stdev());
  printf("Pkt sizes CDF:\n");
  for (const auto &[size, prob] : pkt_sizes_cdf.get_cdf()) {
    printf("             %11lu: %.2f\n", size, prob);
  }
  printf("Total flows:              %s\n", fmt(flows.size()).c_str());
  printf("Total flows (req/res):    %s\n", fmt(symm_flows.size()).c_str());
  printf("Flows without responses:  %s\n",
         fmt(expected_responses.size()).c_str());
  printf("Pkts/flow:                %.2f ± %.2f\n", pkts_per_flow_cdf.get_avg(),
         pkts_per_flow_cdf.get_stdev());
  printf("Pkts/flow CDF:\n");
  for (const auto &[pkts, prob] : pkts_per_flow_cdf.get_cdf()) {
    printf("             %11lu: %.2f\n", pkts, prob);
  }
  printf("Flow duration:            %s ± %s us\n",
         fmt(flow_duration_us_cdf.get_avg()).c_str(),
         fmt(flow_duration_us_cdf.get_stdev()).c_str());
  printf("Flow duration CDF (us):\n");
  for (const auto &[duration, prob] : flow_duration_us_cdf.get_cdf()) {
    printf("             %11lu: %.2f\n", duration, prob);
  }
  printf("Flow time inter-packets:  %s ± %s us\n",
         fmt(flow_dts_us_cdf.get_avg()).c_str(),
         fmt(flow_dts_us_cdf.get_stdev()).c_str());
  printf("Flow time inter-packets CDF (us):\n");
  for (const auto &[gap, prob] : flow_dts_us_cdf.get_cdf()) {
    printf("             %11lu: %.2f\n", gap, prob);
  }

  return 0;
}
