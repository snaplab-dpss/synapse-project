#pragma once

#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibBDD/Profile.h>

#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace LibBDD {

using LibCore::rgb_t;

class BDDProfileVisualizer : public BDDViz {
public:
  BDDProfileVisualizer(const std::string &fname, const bdd_profile_t &profile)
      : BDDViz(bdd_visualizer_opts_t{
            .fname                = fname,
            .colors_per_node      = get_colors_per_node(profile.counters),
            .default_color        = {true, hit_rate_to_color(0_hr)},
            .annotations_per_node = get_annocations_per_node(profile),
            .processed            = processed_t(),
        }) {}

  static void visualize(const BDD *bdd, const bdd_profile_t &profile, bool interrupt) {
    bdd_visualizer_opts_t opts;

    opts.colors_per_node      = get_colors_per_node(profile.counters);
    opts.default_color.first  = true;
    opts.annotations_per_node = get_annocations_per_node(profile);
    opts.default_color.second = hit_rate_to_color(0_hr);

    BDDViz::visualize(bdd, interrupt, opts);
  }

private:
  static u64 get_total_counter(const std::unordered_map<bdd_node_id_t, u64> &counters) {
    u64 total_counter = 0;
    for (auto it = counters.begin(); it != counters.end(); it++) {
      total_counter = std::max(total_counter, it->second);
    }
    return total_counter;
  }

  static std::unordered_map<bdd_node_id_t, std::string> get_annocations_per_node(const bdd_profile_t &profile) {
    const u64 total_counter = get_total_counter(profile.counters);
    std::unordered_map<bdd_node_id_t, std::string> annocations_per_node;

    for (auto it = profile.counters.begin(); it != profile.counters.end(); it++) {
      const bdd_node_id_t node = it->first;
      const u64 counter        = it->second;
      const hit_rate_t node_hit_rate(counter, total_counter);

      std::stringstream ss;
      ss << "HR: " << std::fixed << node_hit_rate;

      for (const auto &[map_addr, map_stats] : profile.stats_per_map) {
        for (const auto &node_map_stats : map_stats.nodes) {
          if (node_map_stats.node != node) {
            continue;
          }

          ss << "\\n";
          ss << "Flows: " << node_map_stats.flows;
        }
      }

      annocations_per_node[node] = ss.str();
    }

    return annocations_per_node;
  }

  static std::unordered_map<bdd_node_id_t, std::string> get_colors_per_node(const std::unordered_map<bdd_node_id_t, u64> &counters) {
    const u64 total_counter = get_total_counter(counters);
    std::unordered_map<bdd_node_id_t, std::string> colors_per_node;

    for (auto it = counters.begin(); it != counters.end(); it++) {
      const bdd_node_id_t node = it->first;
      const u64 counter        = it->second;
      const hit_rate_t node_hit_rate(counter, total_counter);
      const std::string color = hit_rate_to_color(node_hit_rate);

      colors_per_node[node] = color;
    }

    return colors_per_node;
  }

  static std::string hit_rate_to_color(hit_rate_t node_hit_rate) {
    // return hit_rate_to_rainbow(node_hit_rate);
    // return hit_rate_to_blue(node_hit_rate);
    return hit_rate_to_blue_red_scale(node_hit_rate);
  }

  static std::string hit_rate_to_rainbow(hit_rate_t node_hit_rate) {
    const rgb_t blue(0, 0, 1);
    const rgb_t cyan(0, 1, 1);
    const rgb_t green(0, 1, 0);
    const rgb_t yellow(1, 1, 0);
    const rgb_t red(1, 0, 0);

    const std::vector<rgb_t> palette{blue, cyan, green, yellow, red};

    const double value = node_hit_rate.value * (palette.size() - 1);
    const int idx1     = std::floor(value);
    const int idx2     = idx1 + 1;
    const double frac  = value - idx1;

    const u8 r = 0xff * ((palette[idx2].r - palette[idx1].r) * frac + palette[idx1].r);
    const u8 g = 0xff * ((palette[idx2].g - palette[idx1].g) * frac + palette[idx1].g);
    const u8 b = 0xff * ((palette[idx2].b - palette[idx1].b) * frac + palette[idx1].b);

    const rgb_t color(r, g, b);
    return color.to_gv_repr();
  }

  static std::string hit_rate_to_blue(hit_rate_t node_hit_rate) {
    const u8 r = 0xff * (1.0 - node_hit_rate.value);
    const u8 g = 0xff * (1.0 - node_hit_rate.value);
    const u8 b = 0xff;
    const u8 o = 0xff * 0.5;

    const rgb_t color(r, g, b, o);
    return color.to_gv_repr();
  }

  static std::string hit_rate_to_blue_red_scale(hit_rate_t node_hit_rate) {
    const u8 r = 0xff * node_hit_rate.value;
    const u8 g = 0;
    const u8 b = 0xff * (1.0 - node_hit_rate).value;
    const u8 o = 0xff * 0.33;

    const rgb_t color(r, g, b, o);
    return color.to_gv_repr();
  }
};

} // namespace LibBDD