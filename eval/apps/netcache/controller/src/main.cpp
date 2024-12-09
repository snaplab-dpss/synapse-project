#include <signal.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <fstream>
#include <vector>
#include <variant>
#include <chrono>

#include "constants.h"
#include "netcache.h"
#include "listener.h"
#include "process_query.h"

#define REPORT_FILE "netcache-controller.tsv"

struct args_t {
	std::string iface;
	std::string conf_file_path;
	bool use_tofino_model;
	bool bf_prompt;

	args_t(int argc, char **argv) : use_tofino_model(false), bf_prompt(false) {
		if (argc < 4) {
			help(argv);
		}

		parse_help(argc, argv);

		conf_file_path = std::string(argv[1]);

		parse_iface(argc, argv);
		parse_tofino_model_flag(argc, argv);
		parse_bf_prompt_flag(argc, argv);
	}

	void help(char **argv) {
		std::cerr << "Usage: " << argv[0]
				  << " conf -i <listen iface> [--bf-prompt] [--tofino-model] [-h|--help]\n";
		exit(0);
	}

	void parse_iface(int argc, char** argv) {
		auto arg_str = std::string("-i");

		for (auto argi = 1u; argi < argc - 1; argi++) {
			auto arg = std::string(argv[argi]);
			auto cmp = arg.compare(arg_str);

			if (cmp == 0) {
				iface = std::string(argv[argi + 1]);
				std::cout << "iface: " << iface << std::endl;
				return;
			}
		}

		std::cerr << "Option -i not found.\n";
		help(argv);
	}

	void parse_tofino_model_flag(int argc, char **argv) {
		std::string target = "--tofino-model";

		for (auto i = 1; i < argc; i++) {
			auto arg = std::string(argv[i]);
			if (arg.compare(target) == 0) {
				use_tofino_model = true;
				return;
			}
		}
	}

	void parse_bf_prompt_flag(int argc, char **argv) {
		std::string target = "--bf-prompt";

		for (auto i = 1; i < argc; i++) {
			auto arg = std::string(argv[i]);
			if (arg.compare(target) == 0) {
				bf_prompt = true;
				return;
			}
		}
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

	void dump() const {
		std::cout << "\n";
		std::cout << "Configuration:\n";
		std::cout << "  conf (w/ topo): " << conf_file_path << "\n";
		std::cout << "  iface:			" << iface << "\n";
		std::cout << "  model:			" << use_tofino_model << "\n";
		std::cout << "  bf prompt:		" << bf_prompt << "\n";
	}
};

void signalHandler(int signum) {
	auto conf = netcache::Controller::controller->get_conf();
	auto use_tofino_model = netcache::Controller::controller->get_use_tofino_model();

	auto ofs = std::ofstream(REPORT_FILE);

	ofs << "#port\trx bytes\trx packets\ttx packets\n";

	if (!ofs.is_open()) {
		std::cerr << "ERROR: couldn't write to \"" << REPORT_FILE << "\n";
		exit(1);
	}

	for (auto connection : conf.topology.connections) {
		auto in_port = connection.in.port;

		if (!use_tofino_model) {
			in_port = netcache::Controller::controller->get_dev_port(in_port, 0);
		}

		auto tx = netcache::Controller::controller->get_port_tx(in_port);

		ofs << in_port << "\t" << tx << "\n";
	}

	auto stats_port = conf.topology.stats.port;

	if (!use_tofino_model) {
		stats_port =
			netcache::Controller::controller->get_dev_port(stats_port, 0);
	}

	auto tx = netcache::Controller::controller->get_port_tx(stats_port);

	ofs << stats_port << "\t" << tx << "\n";
	ofs.close();

	std::cout << "Report generated at \"" << REPORT_FILE << "\". Exiting.\n";

	exit(signum);
}

int main(int argc, char **argv) {
	signal(SIGINT, signalHandler);
	signal(SIGQUIT, signalHandler);
	signal(SIGTERM, signalHandler);

	args_t args(argc, argv);

	netcache::init_bf_switchd(args.use_tofino_model, args.bf_prompt);
	netcache::setup_controller(args.conf_file_path, args.use_tofino_model);

	args.dump();

	auto listener = netcache::Listener(args.iface);
	auto process_query = netcache::ProcessQuery();

	std::cerr << "\nNetCache controller is ready.\n";

	auto last_time = std::chrono::steady_clock::now();

	while (1) {
		// Check if the reset timers have elapsed.
		// If so, reset the respective counters.
		auto cur_time = std::chrono::steady_clock::now();
		auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(cur_time-last_time);

		if (elapsed_time.count() >= netcache::Controller::controller->conf.key_cntr.reset_timer) {
			netcache::Controller::controller->reg_key_count.set_all_false();
			#ifndef NDEBUG
			std::cout << "Reset timer: data plane reg_key_count." << std::endl;
			#endif
		}

		if (elapsed_time.count() >= netcache::Controller::controller->conf.cm.reset_timer) {
			netcache::Controller::controller->reg_cm_0.set_all_false();
			netcache::Controller::controller->reg_cm_1.set_all_false();
			netcache::Controller::controller->reg_cm_2.set_all_false();
			netcache::Controller::controller->reg_cm_3.set_all_false();
			#ifndef NDEBUG
			std::cout << "Reset timer: data plane reg_cm_0." << std::endl;
			std::cout << "Reset timer: data plane reg_cm_1." << std::endl;
			std::cout << "Reset timer: data plane reg_cm_2." << std::endl;
			std::cout << "Reset timer: data plane reg_cm_3." << std::endl;
			#endif
		}

		if (elapsed_time.count() >= netcache::Controller::controller->conf.bloom.reset_timer) {
			netcache::Controller::controller->reg_bloom_0.set_all_false();
			netcache::Controller::controller->reg_bloom_1.set_all_false();
			netcache::Controller::controller->reg_bloom_2.set_all_false();
			#ifndef NDEBUG
			std::cout << "Reset timer: data plane reg_bloom_0." << std::endl;
			std::cout << "Reset timer: data plane reg_bloom_1." << std::endl;
			std::cout << "Reset timer: data plane reg_bloom_2." << std::endl;
			#endif
		}

		auto query = listener.receive_query();

		if (query.valid) {
			if (query.op == WRITE_QUERY) {
				#ifndef NDEBUG
				printf("WRITE QUERY\n");
				#endif
				process_query.write_query(query);
			} else if (query.op == DELETE_QUERY) {
				#ifndef NDEBUG
				printf("DEL QUERY\n");
				#endif
				process_query.del_query(query);
			} else if (query.op == HOT_READ_QUERY) {
				#ifndef NDEBUG
				printf("HOT READ QUERY\n");
				#endif
				process_query.hot_read_query(query);
			} else {
				std::cerr << "Invalid query received.";
			}
		}
	}

	return 0;
}
