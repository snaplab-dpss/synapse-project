#pragma once

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "bdd_visualizer.h"
#include "../profile.h"
#include "../../util.h"

class BDDProfileVisualizer : public BDDViz {
public:
  BDDProfileVisualizer(const std::string &fname, const bdd_profile_t &profile)
      : BDDViz(bdd_visualizer_opts_t{
            .fname = fname,
            .colors_per_node = get_colors_per_node(profile.counters),
            .default_color = {true, hit_rate_to_color(0)},
            .annotations_per_node = get_annocations_per_node(profile),
            .processed = processed_t(),
        }) {}

  static void visualize(const BDD *bdd, const bdd_profile_t &profile,
                        bool interrupt) {
    bdd_visualizer_opts_t opts;

    opts.colors_per_node = get_colors_per_node(profile.counters);
    opts.default_color.first = true;
    opts.annotations_per_node = get_annocations_per_node(profile);
    opts.default_color.second = hit_rate_to_color(0);

    BDDViz::visualize(bdd, interrupt, opts);
  }

private:
  static u64
  get_total_counter(const std::unordered_map<node_id_t, u64> &counters) {
    u64 total_counter = 0;
    for (auto it = counters.begin(); it != counters.end(); it++) {
      total_counter = std::max(total_counter, it->second);
    }
    return total_counter;
  }

  static std::unordered_map<node_id_t, std::string>
  get_annocations_per_node(const bdd_profile_t &profile) {

    u64 total_counter = get_total_counter(profile.counters);
    std::unordered_map<node_id_t, std::string> annocations_per_node;

    for (auto it = profile.counters.begin(); it != profile.counters.end();
         it++) {
      node_id_t node = it->first;
      u64 counter = it->second;
      hit_rate_t node_hit_rate = (hit_rate_t)counter / total_counter;

      std::stringstream ss;
      ss << "HR: " << std::fixed << node_hit_rate;

      for (const bdd_profile_t::map_stats_t &map_stats : profile.map_stats) {
        if (map_stats.node != node) {
          continue;
        }

        ss << "\\n";
        ss << "Flows: " << int2hr(map_stats.flows);
        ss << "\\n";
        ss << "Avg pkts/flow: " << int2hr(map_stats.avg_pkts_per_flow);
      }

      annocations_per_node[node] = ss.str();
    }

    return annocations_per_node;
  }

  static std::unordered_map<node_id_t, std::string>
  get_colors_per_node(const std::unordered_map<node_id_t, u64> &counters) {
    u64 total_counter = get_total_counter(counters);
    std::unordered_map<node_id_t, std::string> colors_per_node;

    for (auto it = counters.begin(); it != counters.end(); it++) {
      node_id_t node = it->first;
      u64 counter = it->second;
      hit_rate_t node_hit_rate = (hit_rate_t)counter / total_counter;
      std::string color = hit_rate_to_color(node_hit_rate);

      colors_per_node[node] = color;
    }

    return colors_per_node;
  }

  static std::string hit_rate_to_color(float node_hit_rate) {
    // std::string color = hit_rate_to_rainbow(node_hit_rate);
    // std::string color = hit_rate_to_blue(node_hit_rate);
    std::string color = hit_rate_to_blue_red_scale(node_hit_rate);
    return color;
  }

  static std::string hit_rate_to_rainbow(float node_hit_rate) {
    rgb_t blue(0, 0, 1);
    rgb_t cyan(0, 1, 1);
    rgb_t green(0, 1, 0);
    rgb_t yellow(1, 1, 0);
    rgb_t red(1, 0, 0);

    std::vector<rgb_t> palette{blue, cyan, green, yellow, red};

    float value = node_hit_rate * (palette.size() - 1);
    int idx1 = (int)std::floor(value);
    int idx2 = (int)idx1 + 1;
    float frac = value - idx1;

    u8 r =
        0xff * ((palette[idx2].r - palette[idx1].r) * frac + palette[idx1].r);
    u8 g =
        0xff * ((palette[idx2].g - palette[idx1].g) * frac + palette[idx1].g);
    u8 b =
        0xff * ((palette[idx2].b - palette[idx1].b) * frac + palette[idx1].b);

    rgb_t color(r, g, b);
    return color.to_gv_repr();
  }

  static std::string hit_rate_to_blue(float node_hit_rate) {
    u8 r = 0xff * (1 - node_hit_rate);
    u8 g = 0xff * (1 - node_hit_rate);
    u8 b = 0xff;
    u8 o = 0xff * 0.5;

    rgb_t color(r, g, b, o);
    return color.to_gv_repr();
  }

  static std::string hit_rate_to_blue_red_scale(float node_hit_rate) {
    u8 r = 0xff * node_hit_rate;
    u8 g = 0;
    u8 b = 0xff * (1 - node_hit_rate);
    u8 o = 0xff * 0.33;

    rgb_t color(r, g, b, o);
    return color.to_gv_repr();
  }
};
