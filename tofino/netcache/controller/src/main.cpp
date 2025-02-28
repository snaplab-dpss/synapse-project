#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <fstream>
#include <vector>
#include <variant>
#include <chrono>
#include <thread>
#include <CLI/CLI.hpp>
#include <filesystem>

#include "args.h"
#include "constants.h"
#include "netcache.h"
#include "port_stats.h"
#include "process_query.h"
#include "pcie.h"
#include "args.h"

volatile bool stop_reset_timer = false;

std::shared_ptr<netcache::ProcessQuery> netcache::ProcessQuery::process_query;

void reset_counters() {
  auto last_ts_key   = std::chrono::steady_clock::now();
  auto last_ts_cm    = std::chrono::steady_clock::now();
  auto last_ts_bloom = std::chrono::steady_clock::now();

  while (!stop_reset_timer) {
    // Check if the reset timers have elapsed.
    // If so, reset the respective counters.
    auto cur_ts             = std::chrono::steady_clock::now();
    auto elapsed_time_key   = std::chrono::duration_cast<std::chrono::seconds>(cur_ts - last_ts_key);
    auto elapsed_time_cm    = std::chrono::duration_cast<std::chrono::seconds>(cur_ts - last_ts_cm);
    auto elapsed_time_bloom = std::chrono::duration_cast<std::chrono::seconds>(cur_ts - last_ts_bloom);

    if (elapsed_time_key.count() >= netcache::Controller::controller->conf.key_cntr.reset_timer) {
      netcache::Controller::controller->reg_key_count.set_all_false();
      last_ts_key = cur_ts;
#ifndef NDEBUG
      std::cout << "Reset timer: data plane reg_key_count." << std::endl;
#endif
    }

    if (elapsed_time_key.count() >= netcache::Controller::controller->conf.cm.reset_timer) {
      netcache::Controller::controller->reg_cm_0.set_all_false();
      netcache::Controller::controller->reg_cm_1.set_all_false();
      netcache::Controller::controller->reg_cm_2.set_all_false();
      netcache::Controller::controller->reg_cm_3.set_all_false();
      last_ts_cm = cur_ts;
#ifndef NDEBUG
      std::cout << "Reset timer: data plane reg_cm_0." << std::endl;
      std::cout << "Reset timer: data plane reg_cm_1." << std::endl;
      std::cout << "Reset timer: data plane reg_cm_2." << std::endl;
      std::cout << "Reset timer: data plane reg_cm_3." << std::endl;
#endif
    }

    if (elapsed_time_bloom.count() >= netcache::Controller::controller->conf.bloom.reset_timer) {
      netcache::Controller::controller->reg_bloom_0.set_all_false();
      netcache::Controller::controller->reg_bloom_1.set_all_false();
      netcache::Controller::controller->reg_bloom_2.set_all_false();
      last_ts_bloom = cur_ts;
#ifndef NDEBUG
      std::cout << "Reset timer: data plane reg_bloom_0." << std::endl;
      std::cout << "Reset timer: data plane reg_bloom_1." << std::endl;
      std::cout << "Reset timer: data plane reg_bloom_2." << std::endl;
#endif
    }
  }
}

int main(int argc, char **argv) {
  CLI::App app{"KVS server"};

  netcache::args_t args;
  bool dry_run{false};
  bool disable_cache{false};

  app.add_option("--config", args.conf_file_path, "Configuration file")->required();
  app.add_flag("--disable-cache", disable_cache, "Disable cache");
  app.add_flag("--ucli", args.run_ucli, "Interactive user CLI");
  app.add_flag("--wait-ports", args.wait_for_ports,
               "Wait for the ports to be up and running (only relevant when running with the ASIC, not with the model)");
  app.add_flag("--model", args.run_tofino_model, "Run for the tofino model");
  app.add_option("--tna", args.tna_version, "TNA version");
  app.add_option("--ports", args.ports, "Frontend ports")->required();
  app.add_option("--server-port", args.server_port, "Server port")->required();
  app.add_flag("--dry-run", dry_run, "Dry run");

  CLI11_PARSE(app, argc, argv);

  args.cache_activated = !disable_cache;
  assert((args.tna_version == 1 || args.tna_version == 2) && "Invalid TNA version");

  args.dump();

  netcache::conf_t conf = netcache::parse_conf_file(args.conf_file_path);

  if (dry_run) {
    return 0;
  }

  bf_switchd_context_t *switchd_ctx = netcache::init_bf_switchd(args.run_ucli, args.tna_version);
  netcache::setup_controller(conf, args);

  if (args.cache_activated) {
    netcache::register_pcie_pkt_ops();
  }

  netcache::PortStats port_stats;

  auto instance                         = new netcache::ProcessQuery();
  netcache::ProcessQuery::process_query = std::shared_ptr<netcache::ProcessQuery>(instance);

  std::thread reset_thread;
  if (args.cache_activated) {
    reset_thread = std::thread(reset_counters);
  }

  LOG("NetCache controller is ready.");
  DEBUG("Warning: running in debug mode");

  if (args.run_ucli) {
    netcache::run_cli(switchd_ctx);
  } else {
    WAIT_FOR_ENTER("Controller is running. Press enter to terminate.");
  }

  if (args.cache_activated) {
    stop_reset_timer = true;
    reset_thread.join();
  }

  return 0;
}
