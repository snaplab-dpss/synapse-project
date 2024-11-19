#pragma once

#include "query.h"

#include <assert.h>
#include <vector>
#include <fstream>
#include <iostream>

namespace netcache {

struct report_entry_t {
	uint8_t op;
	uint32_t seq;
	uint16_t key;
	uint32_t val;

	report_entry_t(const report_entry_t& _report)
		: op(_report.op),
		  seq(_report.seq),
		  key(_report.key),
		  val(_report.val) {}

	report_entry_t(const query_t& _query)
		: op(_query.op),
		  seq(_query.seq),
		  key(_query.key),
		  val(_query.val) {}
};

struct report_t {
	std::vector<report_entry_t> entries;

	void add(const query_t& query) {
		entries.emplace_back(query);
	}

	void dump(const std::string& report_filename) const {
		auto ofs = std::ofstream(report_filename);

		ofs << "#op\tseq\tkey\tval\n";

		if (!ofs.is_open()) {
			std::cerr << "ERROR: couldn't write to \"" << report_filename << "\n";
			exit(1);
		}

		for (auto entry : entries) {
			ofs << entry.op;
			ofs << "\t";
			ofs << entry.seq;
			ofs << "\t";
			ofs << entry.key;
			ofs << "\t";
			ofs << entry.val;
			ofs << "\t";
		}

		ofs.close();
	}
};

}  // namespace netcache
