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

#include "constants.h"
#include "netcache.h"
#include "port_stats.h"
#include "process_query.h"
#include "pcie.h"

volatile bool stop_reset_timer = false;
volatile bool stop_query_listener = false;

std::shared_ptr<netcache::ProcessQuery> netcache::ProcessQuery::process_query;

struct args_t {
	std::string conf_file_path;
	bool no_cache_activated;

	args_t(int argc, char **argv) : no_cache_activated(false) {
		parse_help(argc, argv);
		parse_conf(argc, argv);
		parse_no_cache_activated(argc, argv);
	}

	void help(char **argv) {
		std::cerr << "Usage: " << argv[0]
				  << " conf -i <listen iface> [--bf-prompt] [--tofino-model] [-h|--help]\n";
		exit(0);
	}

	void parse_help(int argc, char **argv) {
		auto targets = std::vector<std::string>{"-h", "--help"};

		for (auto i = 1; i < argc; i++) {
			auto arg = std::string(argv[i]);
			for (auto target : targets) {
				if (arg.compare(target) == 0) {
					help(argv);
					return;
				}
			}
		}
	}

	void parse_conf(int argc, char **argv) {
		std::string target = "--";

		for (auto i = 1; i < argc; i++) {
			auto arg = std::string(argv[i]);
			if (arg.compare(target) == 0) {
				conf_file_path = std::string(argv[i+1]);
				return;
			}
		}
		std::cout << "Error: No conf file path provided." << std::endl;
		exit(1);
	}

	void parse_no_cache_activated(int argc, char **argv) {
		std::string target = "--no_cache_activated";

		for (auto i = 1; i < argc; i++) {
			auto arg = std::string(argv[i]);
			if (arg.compare(target) == 0) {
				no_cache_activated = true;
				return;
			}
		}
	}
};

void reset_counters(bool no_cache_activated) {
	if (!no_cache_activated) {
		auto last_ts_key	= std::chrono::steady_clock::now();
		auto last_ts_cm		= std::chrono::steady_clock::now();
		auto last_ts_bloom	= std::chrono::steady_clock::now();

		while (!stop_reset_timer) {
			// Check if the reset timers have elapsed.
			// If so, reset the respective counters.
			auto cur_ts = std::chrono::steady_clock::now();
			auto elapsed_time_key =
					std::chrono::duration_cast<std::chrono::seconds>(cur_ts-last_ts_key);
			auto elapsed_time_cm =
					std::chrono::duration_cast<std::chrono::seconds>(cur_ts-last_ts_cm);
			auto elapsed_time_bloom =
					std::chrono::duration_cast<std::chrono::seconds>(cur_ts-last_ts_bloom);

			if (elapsed_time_key.count()
					>= netcache::Controller::controller->conf.key_cntr.reset_timer) {
				netcache::Controller::controller->reg_key_count.set_all_false();
				last_ts_key = cur_ts;
				#ifndef NDEBUG
				std::cout << "Reset timer: data plane reg_key_count." << std::endl;
				#endif
			}

			if (elapsed_time_key.count()
					>= netcache::Controller::controller->conf.cm.reset_timer) {
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

			if (elapsed_time_bloom.count()
					>= netcache::Controller::controller->conf.bloom.reset_timer) {
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
}

/* void query_listener(netcache::ProcessQuery& process_query) { */
/* 	while (!stop_query_listener) { process_query.read_query(); } */
/*  } */

int main(int argc, char **argv) {
	args_t args(argc, argv);

	netcache::conf_t conf = netcache::parse_conf_file(args.conf_file_path);

	netcache::init_bf_switchd(conf.switchd.bf_prompt);
	netcache::setup_controller(conf, conf.switchd.tofino_model);
	netcache::register_pcie_pkt_ops();

	// netcache::ProcessQuery process_query;
	netcache::PortStats port_stats;

	auto instance = new netcache::ProcessQuery();;
	netcache::ProcessQuery::process_query = std::shared_ptr<netcache::ProcessQuery>(instance);

	std::thread reset_thread(reset_counters, args.no_cache_activated);
	// std::thread query_thread(query_listener, std::ref(process_query));

	std::cerr << "\nNetCache controller is ready.\n";

	// Start listening to stdin
	while (1) {
		std::cout << ">";
		std::string command;
		std::getline(std::cin, command);

		if (command == "stats") {
			port_stats.get_stats();
		} else if (command == "reset") {
			port_stats.reset_stats();
		 } else if (command == "exit" || command == "quit") {
			break;
		 } else {
			std::cout << "Wrong command. Avaliable commands: (1) stats; (2) exit/quit.\n";
		 }
	}

	stop_reset_timer = true;
	/* stop_query_listener = true; */

	reset_thread.join();
	/* query_thread.join(); */

	return 0;
}
