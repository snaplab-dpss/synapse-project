#pragma once

#include <assert.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>

namespace netcache {

class Table {
protected:
	const bfrt::BfRtInfo *info;
	std::shared_ptr<bfrt::BfRtSession> session;
	bf_rt_target_t dev_tgt;

	std::string table_name;
	const bfrt::BfRtTable *table;

	std::unique_ptr<bfrt::BfRtTableKey> key;
	std::unique_ptr<bfrt::BfRtTableData> data;
	// std::unique_ptr<bfrt::BfRtTableAttributes> attributes;
	std::unique_ptr<bfrt::BfRtTableEntryScopeArguments> api;

protected:
	Table(const bfrt::BfRtInfo *_info,
		  std::shared_ptr<bfrt::BfRtSession> _session,
		  const bf_rt_target_t &_dev_tgt, const std::string &_table_name)
		: info(_info),
		  session(_session),
		  dev_tgt(_dev_tgt),
		  table_name(_table_name),
		  table(nullptr) {
		assert(_info);
		assert(_session);

		auto bf_status = info->bfrtTableFromNameGet(table_name, &table);
		assert(bf_status == BF_SUCCESS);

		// Allocate key and data once, and use reset across different uses
		bf_status = table->keyAllocate(&key);
		assert(bf_status == BF_SUCCESS);
		assert(key);

		bf_status = table->dataAllocate(&data);
		assert(bf_status == BF_SUCCESS);
		assert(data);
	}

	void init_key(std::unordered_map<std::string, bf_rt_id_t *> fields) {
		for (const auto &field : fields) {
			auto bf_status = table->keyFieldIdGet(field.first, field.second);

			if (bf_status != BF_SUCCESS) {
				std::cerr << "Error configuring table key " << field.first << "\n";
				exit(1);
			}
		}
	}

	void init_data(std::unordered_map<std::string, bf_rt_id_t *> fields) {
		for (const auto &field : fields) {
			auto bf_status = table->dataFieldIdGet(field.first, field.second);

			if (bf_status != BF_SUCCESS) {
				std::cerr << "Error configuring table data " << field.first << "\n";
				exit(1);
			}
		}
	}

	void init_data_with_actions(
		std::unordered_map<std::string, std::pair<bf_rt_id_t, bf_rt_id_t *>>
			fields) {
		for (const auto &field : fields) {
			auto bf_status = table->dataFieldIdGet(
				field.first, field.second.first, field.second.second);

			if (bf_status != BF_SUCCESS) {
				std::cerr << "Error configuring table data with actions " << field.first << "\n";
				exit(1);
			}
		}
	}

	void init_actions(std::unordered_map<std::string, bf_rt_id_t *> actions) {
		for (const auto &action : actions) {
			auto bf_status = table->actionIdGet(action.first, action.second);

			if (bf_status != BF_SUCCESS) {
				std::cerr << "Error configuring table action " << action.first << "\n";
				exit(1);
			}
		}
	}

	size_t get_size() const {
		size_t size;
		auto bf_status = table->tableSizeGet(*session, dev_tgt, &size);
		assert(bf_status == BF_SUCCESS);
		return size;
	}

	size_t get_usage() const {
		uint32_t usage;
		auto bf_status = table->tableUsageGet(
			*session, dev_tgt, bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW,
			&usage);
		assert(bf_status == BF_SUCCESS);
		return usage;
	}

public:
	void dump_data_fields();
	void dump_data_fields(std::ostream &);

	void dump();
	void dump(std::ostream &);

	static void dump_table_names(const bfrt::BfRtInfo *bfrtInfo);
};
};	// namespace netcache
