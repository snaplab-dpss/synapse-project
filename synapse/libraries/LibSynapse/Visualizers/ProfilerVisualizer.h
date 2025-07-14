#pragma once

#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibSynapse/Profiler.h>

#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace LibSynapse {

using LibCore::rgb_t;

using LibBDD::bdd_visualizer_opts_t;
using LibBDD::BDDNodeType;
using LibBDD::BDDNodeVisitAction;
using LibBDD::BDDViz;

class ProfilerViz : public BDDViz {
public:
  static void visualize(const BDD *bdd, const Profiler &profiler, bool interrupt) {
    const std::unordered_map<bdd_node_id_t, hit_rate_t> hrpn = hr_per_node(bdd, profiler);

    bdd_visualizer_opts_t opts;

    opts.colors_per_node      = get_colors_per_node(hrpn);
    opts.default_color.first  = true;
    opts.annotations_per_node = get_annocations_per_node(bdd, profiler, hrpn);
    opts.default_color.second = fraction_to_color(0_hr);

    BDDViz::visualize(bdd, interrupt, opts);
  }

  static void dump_to_file(const BDD *bdd, const Profiler &profiler, const std::filesystem::path &file_name) {
    assert(bdd && "Invalid BDD");

    const std::unordered_map<bdd_node_id_t, hit_rate_t> hrpn = hr_per_node(bdd, profiler);

    bdd_visualizer_opts_t opts;
    opts.fname                = file_name;
    opts.colors_per_node      = get_colors_per_node(hrpn);
    opts.default_color.first  = true;
    opts.annotations_per_node = get_annocations_per_node(bdd, profiler, hrpn);
    opts.default_color.second = fraction_to_color(0_hr);

    BDDViz visualizer(opts);
    visualizer.visit(bdd);
    visualizer.write();
  }

private:
  static std::unordered_map<bdd_node_id_t, hit_rate_t> hr_per_node(const BDD *bdd, const Profiler &profiler) {
    std::unordered_map<bdd_node_id_t, hit_rate_t> fractions_per_node;
    const BDDNode *root = bdd->get_root();

    root->visit_nodes([&fractions_per_node, profiler](const BDDNode *node) {
      const hit_rate_t fraction          = profiler.get_hr(node);
      fractions_per_node[node->get_id()] = fraction;
      return BDDNodeVisitAction::Continue;
    });

    return fractions_per_node;
  }

  static std::unordered_map<bdd_node_id_t, std::string> get_annocations_per_node(const BDD *bdd, const Profiler &profiler,
                                                                                 const std::unordered_map<bdd_node_id_t, hit_rate_t> &hrpn) {
    std::unordered_map<bdd_node_id_t, std::string> annocations_per_node;

    for (const auto &[node_id, fraction] : hrpn) {
      std::stringstream ss;
      ss << "HR=" << fraction;

      const BDDNode *bdd_node = bdd->get_node_by_id(node_id);
      if (bdd_node->get_type() == BDDNodeType::Route) {
        const fwd_stats_t fwd_stats = profiler.get_fwd_stats(bdd_node);
        ss << "\n";
        switch (fwd_stats.operation) {
        case RouteOp::Drop: {
          ss << "Drop={" << fwd_stats.drop << "}";
        } break;
        case RouteOp::Broadcast: {
          ss << "Bcast={" << fwd_stats.flood << "}";
        } break;
        case RouteOp::Forward: {
          ss << "Fwd={";
          int i = 0;
          for (const auto &[port, hr] : fwd_stats.ports) {
            if (hr == 0) {
              continue;
            }
            if (i != 0) {
              ss << ",";
            }
            if (i % 4 == 0) {
              ss << "\n";
            }
            ss << port << ":" << hr;
            i++;
          }
          ss << "}";
        } break;
        }
      }

      annocations_per_node[node_id] = ss.str();
    }

    return annocations_per_node;
  }

  static std::unordered_map<bdd_node_id_t, std::string> get_colors_per_node(const std::unordered_map<bdd_node_id_t, hit_rate_t> &fraction_per_node) {
    std::unordered_map<bdd_node_id_t, std::string> colors_per_node;

    for (const auto &[node, fraction] : fraction_per_node) {
      const std::string color = fraction_to_color(fraction);
      colors_per_node[node]   = color;
    }

    return colors_per_node;
  }

  static std::string fraction_to_color(hit_rate_t fraction) {
    // return hit_rate_to_rainbow(fraction);
    // return hit_rate_to_blue(fraction);
    return hit_rate_to_blue_red_scale(fraction);
  }

  static std::string hit_rate_to_rainbow(hit_rate_t fraction) {
    const rgb_t blue(0, 0, 1);
    const rgb_t cyan(0, 1, 1);
    const rgb_t green(0, 1, 0);
    const rgb_t yellow(1, 1, 0);
    const rgb_t red(1, 0, 0);

    const std::vector<rgb_t> palette{blue, cyan, green, yellow, red};

    double value = fraction.value * (palette.size() - 1);
    int idx1     = std::floor(value);
    int idx2     = idx1 + 1;
    double frac  = value - idx1;

    const u8 r = 0xff * ((palette[idx2].r - palette[idx1].r) * frac + palette[idx1].r);
    const u8 g = 0xff * ((palette[idx2].g - palette[idx1].g) * frac + palette[idx1].g);
    const u8 b = 0xff * ((palette[idx2].b - palette[idx1].b) * frac + palette[idx1].b);

    const rgb_t color(r, g, b);
    return color.to_gv_repr();
  }

  static std::string hit_rate_to_blue(hit_rate_t fraction) {
    const u8 r = 0xff * (1 - fraction.value);
    const u8 g = 0xff * (1 - fraction.value);
    const u8 b = 0xff;
    const u8 o = 0xff * 0.5;

    const rgb_t color(r, g, b, o);
    return color.to_gv_repr();
  }

  static std::string hit_rate_to_blue_red_scale(hit_rate_t fraction) {
    const u8 r = 0xff * fraction.value;
    const u8 g = 0;
    const u8 b = 0xff * (1 - fraction.value);
    const u8 o = 0xff * 0.33;

    const rgb_t color(r, g, b, o);
    return color.to_gv_repr();
  }
};

} // namespace LibSynapse