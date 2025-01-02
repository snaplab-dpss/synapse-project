#pragma once

#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "../bdd/bdd.h"
#include "../profiler.h"
#include "../util.h"

namespace synapse {
class ProfilerViz : public BDDViz {
public:
  static void visualize(const BDD *bdd, const Profiler &profiler, bool interrupt) {
    std::unordered_map<node_id_t, hit_rate_t> hrpn = hr_per_node(bdd, profiler);

    bdd_visualizer_opts_t opts;

    opts.colors_per_node = get_colors_per_node(hrpn);
    opts.default_color.first = true;
    opts.annotations_per_node = get_annocations_per_node(bdd, profiler, hrpn);
    opts.default_color.second = fraction_to_color(0);

    BDDViz::visualize(bdd, interrupt, opts);
  }

private:
  static std::unordered_map<node_id_t, hit_rate_t> hr_per_node(const BDD *bdd,
                                                               const Profiler &profiler) {
    std::unordered_map<node_id_t, hit_rate_t> fractions_per_node;
    const Node *root = bdd->get_root();

    root->visit_nodes([&fractions_per_node, profiler](const Node *node) {
      hit_rate_t fraction = profiler.get_hr(node);
      fractions_per_node[node->get_id()] = fraction;
      return NodeVisitAction::Continue;
    });

    return fractions_per_node;
  }

  static std::unordered_map<node_id_t, std::string>
  get_annocations_per_node(const BDD *bdd, const Profiler &profiler,
                           const std::unordered_map<node_id_t, hit_rate_t> &hrpn) {
    std::unordered_map<node_id_t, std::string> annocations_per_node;

    for (const auto &[node, fraction] : hrpn) {
      std::string color = fraction_to_color(fraction);
      std::stringstream ss;
      ss << "HR: " << std::fixed << fraction;
      annocations_per_node[node] = ss.str();
    }

    return annocations_per_node;
  }

  static std::unordered_map<node_id_t, std::string> get_colors_per_node(
      const std::unordered_map<node_id_t, hit_rate_t> &fraction_per_node) {
    std::unordered_map<node_id_t, std::string> colors_per_node;

    for (const auto &[node, fraction] : fraction_per_node) {
      std::string color = fraction_to_color(fraction);
      colors_per_node[node] = color;
    }

    return colors_per_node;
  }

  static std::string fraction_to_color(hit_rate_t fraction) {
    // std::string color = hit_rate_to_rainbow(fraction);
    // std::string color = hit_rate_to_blue(fraction);
    std::string color = hit_rate_to_blue_red_scale(fraction);
    return color;
  }

  static std::string hit_rate_to_rainbow(hit_rate_t fraction) {
    rgb_t blue(0, 0, 1);
    rgb_t cyan(0, 1, 1);
    rgb_t green(0, 1, 0);
    rgb_t yellow(1, 1, 0);
    rgb_t red(1, 0, 0);

    std::vector<rgb_t> palette{blue, cyan, green, yellow, red};

    hit_rate_t value = fraction * (palette.size() - 1);
    int idx1 = (int)std::floor(value);
    int idx2 = (int)idx1 + 1;
    hit_rate_t frac = value - idx1;

    u8 r = 0xff * ((palette[idx2].r - palette[idx1].r) * frac + palette[idx1].r);
    u8 g = 0xff * ((palette[idx2].g - palette[idx1].g) * frac + palette[idx1].g);
    u8 b = 0xff * ((palette[idx2].b - palette[idx1].b) * frac + palette[idx1].b);

    rgb_t color(r, g, b);
    return color.to_gv_repr();
  }

  static std::string hit_rate_to_blue(hit_rate_t fraction) {
    u8 r = 0xff * (1 - fraction);
    u8 g = 0xff * (1 - fraction);
    u8 b = 0xff;
    u8 o = 0xff * 0.5;

    rgb_t color(r, g, b, o);
    return color.to_gv_repr();
  }

  static std::string hit_rate_to_blue_red_scale(hit_rate_t fraction) {
    u8 r = 0xff * fraction;
    u8 g = 0;
    u8 b = 0xff * (1 - fraction);
    u8 o = 0xff * 0.33;

    rgb_t color(r, g, b, o);
    return color.to_gv_repr();
  }
};
} // namespace synapse