#include <unordered_set>
#include <unordered_map>
#include <map>
#include <filesystem>
#include <optional>
#include <time.h>
#include <fstream>

#include <nlohmann/json.hpp>
#include <CLI/CLI.hpp>

#include "../src/pcap.h"

#define EPOCH_DURATION_NS 1'000'000'000 // 1 second

using namespace synapse;
using json = nlohmann::json;

class Clock {
private:
  bool on;
  time_ns_t alarm;
  time_ns_t interval;

public:
  Clock(time_ns_t _interval) : on(false), alarm(0), interval(_interval) {}

  bool tick(time_ns_t now) {
    bool sound_alarm = false;

    if (!on) {
      on    = true;
      alarm = now + interval;
    } else if (now >= alarm) {
      alarm       = now + interval;
      sound_alarm = true;
    }

    return sound_alarm;
  }
};

class CDF {
private:
  std::map<u64, u64> values;
  u64 total;

public:
  CDF() : total(0) {}

  void add(u64 value) {
    values[value]++;
    total++;
  }

  void add(u64 value, u64 count) {
    values[value] += count;
    total += count;
  }

  std::map<u64, double> get_cdf() const {
    std::map<u64, double> cdf;
    u64 accounted = 0;

    double next_p = 0;
    double step   = 0.05;

    for (const auto &[value, count] : values) {
      accounted += count;

      if (accounted == total) {
        cdf[value] = 1;
        break;
      }

      double p = static_cast<double>(accounted) / total;

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
    double avg   = get_avg();
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

struct report_t {
  time_ns_t start;
  time_ns_t end;
  u64 total_pkts;
  u64 tcpudp_pkts;
  CDF pkt_sizes_cdf;
  u64 total_flows;
  u64 total_symm_flows;
  CDF concurrent_flows_per_epoch;
  CDF pkts_per_flow_cdf;
  CDF top_k_flows_cdf;
  CDF top_k_flows_bytes_cdf;
  CDF flow_duration_us_cdf;
  CDF flow_dts_us_cdf;
};

void dump_report(const std::filesystem::path &output_report, const report_t &report) {
  json j;
  j["start_utc_ns"]                   = report.start;
  j["end_utc_ns"]                     = report.end;
  j["total_pkts"]                     = report.total_pkts;
  j["tcpudp_pkts"]                    = report.tcpudp_pkts;
  j["pkt_bytes_avg"]                  = report.pkt_sizes_cdf.get_avg();
  j["pkt_bytes_stdev"]                = report.pkt_sizes_cdf.get_stdev();
  j["pkt_bytes_cdf"]                  = json();
  j["pkt_bytes_cdf"]["values"]        = json::array();
  j["pkt_bytes_cdf"]["probabilities"] = json::array();
  for (const auto &[v, p] : report.pkt_sizes_cdf.get_cdf()) {
    j["pkt_bytes_cdf"]["values"].push_back(v);
    j["pkt_bytes_cdf"]["probabilities"].push_back(p);
  }
  j["total_flows"]                        = report.total_flows;
  j["total_symm_flows"]                   = report.total_symm_flows;
  j["pkts_per_flow_avg"]                  = report.pkts_per_flow_cdf.get_avg();
  j["pkts_per_flow_stdev"]                = report.pkts_per_flow_cdf.get_stdev();
  j["pkts_per_flow_cdf"]                  = json();
  j["pkts_per_flow_cdf"]["values"]        = json::array();
  j["pkts_per_flow_cdf"]["probabilities"] = json::array();
  for (const auto &[v, p] : report.pkts_per_flow_cdf.get_cdf()) {
    j["pkts_per_flow_cdf"]["values"].push_back(v);
    j["pkts_per_flow_cdf"]["probabilities"].push_back(p);
  }
  j["flow_duration_us_avg"]                  = report.flow_duration_us_cdf.get_avg();
  j["flow_duration_us_stdev"]                = report.flow_duration_us_cdf.get_stdev();
  j["flow_duration_us_cdf"]                  = json();
  j["flow_duration_us_cdf"]["values"]        = json::array();
  j["flow_duration_us_cdf"]["probabilities"] = json::array();
  for (const auto &[v, p] : report.flow_duration_us_cdf.get_cdf()) {
    j["flow_duration_us_cdf"]["values"].push_back(v);
    j["flow_duration_us_cdf"]["probabilities"].push_back(p);
  }
  j["flow_dts_us_avg"]                  = report.flow_dts_us_cdf.get_avg();
  j["flow_dts_us_stdev"]                = report.flow_dts_us_cdf.get_stdev();
  j["flow_dts_us_cdf"]                  = json();
  j["flow_dts_us_cdf"]["values"]        = json::array();
  j["flow_dts_us_cdf"]["probabilities"] = json::array();
  for (const auto &[v, p] : report.flow_dts_us_cdf.get_cdf()) {
    j["flow_dts_us_cdf"]["values"].push_back(v);
    j["flow_dts_us_cdf"]["probabilities"].push_back(p);
  }
  j["top_k_flows_cdf"]                  = json();
  j["top_k_flows_cdf"]["values"]        = json::array();
  j["top_k_flows_cdf"]["probabilities"] = json::array();
  for (const auto &[v, p] : report.top_k_flows_cdf.get_cdf()) {
    j["top_k_flows_cdf"]["values"].push_back(v);
    j["top_k_flows_cdf"]["probabilities"].push_back(p);
  }
  j["top_k_flows_bytes_cdf"]                  = json();
  j["top_k_flows_bytes_cdf"]["values"]        = json::array();
  j["top_k_flows_bytes_cdf"]["probabilities"] = json::array();
  for (const auto &[v, p] : report.top_k_flows_bytes_cdf.get_cdf()) {
    j["top_k_flows_bytes_cdf"]["values"].push_back(v);
    j["top_k_flows_bytes_cdf"]["probabilities"].push_back(p);
  }

  fprintf(stderr, "\n");
  fprintf(stderr, "Dumping report to %s\n", output_report.c_str());

  std::ofstream out(output_report);
  out << j.dump(2) << std::endl;
}

void print_report(const report_t &report) {
  printf("###################### PCAP Stats ######################\n");
  printf("Start:                    %s\n", fmt_time_hh(report.start).c_str());
  printf("End:                      %s\n", fmt_time_hh(report.end).c_str());
  printf("Duration:                 %s\n", fmt_time_duration_hh(report.start, report.end).c_str());
  printf("Total packets:            %s\n", fmt(report.total_pkts).c_str());
  printf("Total TCP/UDP packets:    %s (%d%%)\n", fmt(report.tcpudp_pkts).c_str(),
         (int)(100.0 * report.tcpudp_pkts / report.total_pkts));
  printf("Pkt sizes:                %.2f ± %.2f B\n", report.pkt_sizes_cdf.get_avg(), report.pkt_sizes_cdf.get_stdev());
  printf("Pkt sizes CDF:\n");
  for (const auto &[size, prob] : report.pkt_sizes_cdf.get_cdf()) {
    printf("             %11lu: %.2lf\n", size, prob);
  }
  printf("Total flows:              %s\n", fmt(report.total_flows).c_str());
  printf("Total symmetric flows:    %s\n", fmt(report.total_symm_flows).c_str());
  printf("Concurrent flows/epoch:   %s ± %s\n", fmt(report.concurrent_flows_per_epoch.get_avg()).c_str(),
         fmt(report.concurrent_flows_per_epoch.get_stdev()).c_str());
  printf("Pkts/flow:                %.2f ± %.2f\n", report.pkts_per_flow_cdf.get_avg(),
         report.pkts_per_flow_cdf.get_stdev());
  printf("Pkts/flow CDF:\n");
  for (const auto &[pkts, prob] : report.pkts_per_flow_cdf.get_cdf()) {
    printf("             %11lu: %.2f\n", pkts, prob);
  }
  printf("Flow duration:            %s ± %s us\n", fmt(report.flow_duration_us_cdf.get_avg()).c_str(),
         fmt(report.flow_duration_us_cdf.get_stdev()).c_str());
  printf("Flow duration CDF (us):\n");
  for (const auto &[duration, prob] : report.flow_duration_us_cdf.get_cdf()) {
    printf("             %11lu: %.2f\n", duration, prob);
  }
  printf("Flow time inter-packets:  %s ± %s us\n", fmt(report.flow_dts_us_cdf.get_avg()).c_str(),
         fmt(report.flow_dts_us_cdf.get_stdev()).c_str());
  printf("Flow time inter-packets CDF (us):\n");
  for (const auto &[gap, prob] : report.flow_dts_us_cdf.get_cdf()) {
    printf("             %11lu: %.2f\n", gap, prob);
  }
  printf("Top-k flows (#pkts) CDF:\n");
  for (const auto &[k, prob] : report.top_k_flows_cdf.get_cdf()) {
    printf("             %11lu: %.2f (%d%%)\n", k, prob, (int)(k * 100.0 / report.total_flows));
  }
  printf("Top-k flows (bytes) CDF:\n");
  for (const auto &[k, prob] : report.top_k_flows_bytes_cdf.get_cdf()) {
    printf("             %11lu: %.2f (%d%%)\n", k, prob, (int)(k * 100.0 / report.total_flows));
  }
}

int main(int argc, char *argv[]) {
  CLI::App app{"Pcap stats"};

  std::filesystem::path pcap_file;
  std::filesystem::path output_report;

  app.add_option("pcap", pcap_file, "Pcap file.")->required();
  app.add_option("--out", output_report, "Output report JSON file.");

  CLI11_PARSE(app, argc, argv);

  if (!std::filesystem::exists(pcap_file)) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    exit(1);
  }

  PcapReader pcap_reader(argv[1]);

  Clock clock(EPOCH_DURATION_NS);
  std::unordered_set<flow_t, flow_t::flow_hash_t> flows;
  std::unordered_set<sflow_t, sflow_t::flow_hash_t> symm_flows;
  std::vector<std::unordered_set<flow_t, flow_t::flow_hash_t>> concurrent_flows_per_epoch;

  std::unordered_map<flow_t, u64, sflow_t::flow_hash_t> pkts_per_flow;
  std::unordered_map<flow_t, u64, sflow_t::flow_hash_t> bytes_per_flow;
  std::unordered_map<flow_t, flow_ts, sflow_t::flow_hash_t> flow_times;

  report_t report;

  report.total_pkts  = pcap_reader.get_total_pkts();
  report.tcpudp_pkts = 0;
  concurrent_flows_per_epoch.emplace_back();

  u64 pkt_count = 0;
  int progress  = -1;

  const u8 *pkt;
  u16 hdrs_len;
  u16 sz;
  time_ns_t ts;
  std::optional<flow_t> flow;

  while (pcap_reader.read(pkt, hdrs_len, sz, ts, flow)) {
    pkt_count++;

    int current_progress = 100.0 * pkt_count / report.total_pkts;

    if (current_progress > progress) {
      progress = current_progress;
      fprintf(stderr, "\r[Progress %3d%%]", progress);
      fflush(stderr);
    }

    report.pkt_sizes_cdf.add(sz);

    if (!flow.has_value()) {
      continue;
    }

    if (clock.tick(ts)) {
      concurrent_flows_per_epoch.emplace_back();
    }

    report.tcpudp_pkts++;
    flows.insert(flow.value());
    symm_flows.insert(flow.value());
    concurrent_flows_per_epoch.back().insert(flow.value());
    pkts_per_flow[flow.value()]++;
    bytes_per_flow[flow.value()] += sz;

    auto flow_times_it = flow_times.find(flow.value());

    if (flow_times_it == flow_times.end()) {
      flow_times[flow.value()] = {
          .first = ts,
          .last  = ts,
          .dts   = {},
      };
    } else {
      flow_ts &fts = flow_times_it->second;
      time_ns_t dt = ts - fts.last;
      fts.last     = ts;
      fts.dts.push_back(dt);
    }
  }

  report.start            = pcap_reader.get_start();
  report.end              = pcap_reader.get_end();
  report.total_flows      = flows.size();
  report.total_symm_flows = symm_flows.size();

  for (const auto &flows : concurrent_flows_per_epoch) {
    report.concurrent_flows_per_epoch.add(flows.size());
  }

  std::vector<u64> pkts_per_flow_values;
  std::vector<u64> bytes_per_flow_values;

  for (const auto &[flow, pkts] : pkts_per_flow) {
    report.pkts_per_flow_cdf.add(pkts);
    pkts_per_flow_values.push_back(pkts);
  }

  for (const auto &[flow, bytes] : bytes_per_flow) {
    bytes_per_flow_values.push_back(bytes);
  }

  assert(pkts_per_flow_values.size() == bytes_per_flow_values.size());

  std::sort(pkts_per_flow_values.begin(), pkts_per_flow_values.end(), std::greater<u64>());
  std::sort(bytes_per_flow_values.begin(), bytes_per_flow_values.end(), std::greater<u64>());

  for (size_t i = 0; i < pkts_per_flow_values.size(); i++) {
    report.top_k_flows_cdf.add(i + 1, pkts_per_flow_values[i]);
    report.top_k_flows_bytes_cdf.add(i + 1, bytes_per_flow_values[i]);
  }

  for (const auto &[flow, ts] : flow_times) {
    report.flow_duration_us_cdf.add((ts.last - ts.first) / THOUSAND);

    if (ts.dts.empty()) {
      continue;
    }

    time_us_t dt_sum = 0;
    for (const auto &dt : ts.dts) {
      dt_sum += dt / THOUSAND;
    }
    report.flow_dts_us_cdf.add(dt_sum / (double)ts.dts.size());
  }

  print_report(report);

  if (!output_report.empty()) {
    dump_report(output_report, report);
  }

  return 0;
}
