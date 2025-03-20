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
#include "process_query.h"
#include "pcie.h"
#include "args.h"
#include "log.h"

volatile bool stop_reset_timer = false;

std::shared_ptr<netcache::ProcessQuery> netcache::ProcessQuery::process_query;

void reset_counters() {
  auto last_ts = std::chrono::steady_clock::now();

  while (!stop_reset_timer) {
    auto cur_ts       = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(cur_ts - last_ts);

    if (elapsed_time.count() >= netcache::Controller::controller->args.reset_timer_sec) {
      DEBUG("Resetting timers");

      netcache::Controller::controller->begin_transaction();

      netcache::Controller::controller->reg_key_count.set_all_false();

      netcache::Controller::controller->reg_cm_0.set_all_false();
      netcache::Controller::controller->reg_cm_1.set_all_false();
      netcache::Controller::controller->reg_cm_2.set_all_false();
      netcache::Controller::controller->reg_cm_3.set_all_false();

      netcache::Controller::controller->reg_bloom_0.set_all_false();
      netcache::Controller::controller->reg_bloom_1.set_all_false();
      netcache::Controller::controller->reg_bloom_2.set_all_false();

      netcache::Controller::controller->end_transaction();
      DEBUG("Reset timers done");

      last_ts = cur_ts;
    }
  }
}

int main(int argc, char **argv) {
  CLI::App app{"KVS server"};

  netcache::args_t args;
  bool dry_run{false};
  bool disable_cache{false};

  app.add_option("--client-ports", args.client_ports, "Frontend client ports")->required();
  app.add_option("--server-port", args.server_port, "Frontend server port")->required();
  app.add_flag("--disable-cache", disable_cache, "Disable cache");
  app.add_flag("--ucli", args.run_ucli, "Interactive user CLI");
  app.add_flag("--wait-ports", args.wait_for_ports,
               "Wait for the ports to be up and running (only relevant when running with the ASIC, not with the model)");
  app.add_flag("--model", args.run_tofino_model, "Run for the tofino model");
  app.add_option("--tna", args.tna_version, "TNA version");
  app.add_option("--sample-size", args.sample_size, "Number of entries periodically probed from the data plane")->default_val(50);
  app.add_option("--reset-timers", args.reset_timer_sec, "Reset timer in seconds")->default_val(3);
  app.add_flag("--dry-run", dry_run, "Dry run");

  CLI11_PARSE(app, argc, argv);

  args.cache_activated = !disable_cache;
  assert((args.tna_version == 1 || args.tna_version == 2) && "Invalid TNA version");

  args.dump();

  if (dry_run) {
    return 0;
  }

  bf_switchd_context_t *switchd_ctx = netcache::init_bf_switchd(args.run_ucli, args.tna_version);
  netcache::setup_controller(args);

  if (args.cache_activated) {
    netcache::register_pcie_pkt_ops();
  }

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
