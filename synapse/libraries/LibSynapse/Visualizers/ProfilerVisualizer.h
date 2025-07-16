#pragma once

#include <LibBDD/Visitors/BDDVisualizer.h>
#include <LibSynapse/Profiler.h>

#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace LibSynapse {

using LibCore::TreeViz;
using LibCore::Graphviz::Color;

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
    opts.annotations_per_node = get_annocations_per_node(bdd, profiler, hrpn);
    opts.default_color        = fraction_to_color(0_hr);

    BDDViz::visualize(bdd, interrupt, opts);
  }

  static void dump_to_file(const BDD *bdd, const Profiler &profiler, const std::filesystem::path &file_name) {
    assert(bdd && "Invalid BDD");

    const std::unordered_map<bdd_node_id_t, hit_rate_t> hrpn = hr_per_node(bdd, profiler);

    bdd_visualizer_opts_t opts;
    opts.fname                = file_name;
    opts.colors_per_node      = get_colors_per_node(hrpn);
    opts.annotations_per_node = get_annocations_per_node(bdd, profiler, hrpn);
    opts.default_color        = fraction_to_color(0_hr);

    BDDViz::dump_to_file(bdd, opts);
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

  static std::unordered_map<bdd_node_id_t, Color> get_colors_per_node(const std::unordered_map<bdd_node_id_t, hit_rate_t> &fraction_per_node) {
    std::unordered_map<bdd_node_id_t, Color> colors_per_node;

    for (const auto &[node, fraction] : fraction_per_node) {
      colors_per_node[node] = fraction_to_color(fraction);
    }

    return colors_per_node;
  }

  static Color fraction_to_color(hit_rate_t fraction) {
    // return hit_rate_to_rainbow(fraction);
    // return hit_rate_to_blue(fraction);
    return hit_rate_to_blue_red_scale(fraction);
  }

  static Color hit_rate_to_rainbow(hit_rate_t fraction) {
    const Color blue(0, 0, 1);
    const Color cyan(0, 1, 1);
    const Color green(0, 1, 0);
    const Color yellow(1, 1, 0);
    const Color red(1, 0, 0);

    const std::vector<Color> palette{blue, cyan, green, yellow, red};

    double value = fraction.value * (palette.size() - 1);
    int idx1     = std::floor(value);
    int idx2     = idx1 + 1;
    double frac  = value - idx1;

    const u8 r = 0xff * ((palette[idx2].rgb.red - palette[idx1].rgb.red) * frac + palette[idx1].rgb.red);
    const u8 g = 0xff * ((palette[idx2].rgb.green - palette[idx1].rgb.green) * frac + palette[idx1].rgb.green);
    const u8 b = 0xff * ((palette[idx2].rgb.blue - palette[idx1].rgb.blue) * frac + palette[idx1].rgb.blue);

    return Color(r, g, b);
  }

  static Color hit_rate_to_blue(hit_rate_t fraction) {
    const u8 r = 0xff * (1 - fraction.value);
    const u8 g = 0xff * (1 - fraction.value);
    const u8 b = 0xff;
    const u8 o = 0xff * 0.5;
    return Color(r, g, b, o);
  }

  static Color hit_rate_to_blue_red_scale(hit_rate_t fraction) {
    const u8 r = 0xff * fraction.value;
    const u8 g = 0;
    const u8 b = 0xff * (1 - fraction.value);
    const u8 o = 0xff * 0.33;
    return Color(r, g, b, o);
  }
};

} // namespace LibSynapse