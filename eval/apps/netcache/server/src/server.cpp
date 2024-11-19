#include <iostream>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <signal.h>

#include "constants.h"
#include "listener.h"
#include "report.h"
#include "store.h"

#define REPORT_FILE "netcache-server.tsv"

netcache::report_t report;

struct args_t {
	std::string iface;

	args_t(int argc, char** argv) {
		if (argc < 3) {
			usage(argv);
		}

		parse_help(argc, argv);
		parse_iface(argc, argv);
	}

	void usage(char** argv) const {
		std::cerr << "Usage: " << argv[0]
				  << " -i <listen iface> [-h|--help]\n";
		exit(1);
	}

	void parse_help(int argc, char** argv) {
		auto args_str = std::vector<std::string>{
			std::string("-h"),
			std::string("--help"),
		};

		for (auto argi = 1u; argi < argc - 1; argi++) {
			auto arg = std::string(argv[argi]);

			for (auto arg_str : args_str) {
				auto cmp = arg.compare(arg_str);

				if (cmp == 0) {
					usage(argv);
				}
			}
		}
	}

	void parse_iface(int argc, char** argv) {
		auto arg_str = std::string("-i");

		for (auto argi = 1u; argi < argc - 1; argi++) {
			auto arg = std::string(argv[argi]);
			auto cmp = arg.compare(arg_str);

			if (cmp == 0) {
				iface = std::string(argv[argi + 1]);
				return;
			}
		}

		std::cerr << "Option -i not found.\n";
		usage(argv);
	}

	void dump() const {
		std::cerr << "Configuration:\n";
		std::cerr << "  listen iface: " << iface << "\n";
		std::cerr << "\n";
	}
};

void signalHandler(int signum) {
	report.dump(REPORT_FILE);

	std::cout << "Report generated at \"" << REPORT_FILE << "\". Exiting.\n";
	exit(signum);
}

int main(int argc, char** argv) {
	signal(SIGINT, signalHandler);
	signal(SIGQUIT, signalHandler);
	signal(SIGTERM, signalHandler);

	auto args = args_t(argc, argv);

	args.dump();

	auto listener = netcache::Listener(args.iface);
	auto store = netcache::Store();

	while (1) {
		auto query = listener.receive_query();

		if (query.valid) {
			if (query.op == WRITE_QUERY) {
				store.write_query(query);
			} else if (query.op == DELETE_QUERY) {
				store.del_query(query);
			} else if (query.op == HOT_READ_QUERY) {
				store.hot_read_query(query);
			} else {
				std::cerr << "Invalid query received.";
			}
		}
	}

	return 0;
}
