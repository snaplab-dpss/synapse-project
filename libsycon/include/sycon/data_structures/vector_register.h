#pragma once

#include <array>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "synapse_ds.h"
#include "../config.h"
#include "../constants.h"
#include "../primitives/register.h"
#include "../time.h"
#include "../buffer.h"

namespace sycon {

// The NF clears every vector of an interval in the same packet (vector_periodic_clear is called on
// each with the one `now`), so their intervals stay in lockstep: one thread per interval clears
// all its registers in one transaction, which a thread per register, each with its own phase,
// would not.
class PeriodicClear {
private:
  std::mutex mutex;
  std::map<time_ms_t, std::vector<std::function<void()>>> clears_by_interval;

  PeriodicClear() = default;

  void tick(time_ms_t interval) {
    const std::lock_guard<std::mutex> lock(mutex);
    cfg.begin_transaction();
    for (const std::function<void()> &clear : clears_by_interval[interval]) {
      clear();
    }
    cfg.commit_transaction();
  }

public:
  static PeriodicClear &instance() {
    static PeriodicClear periodic_clear;
    return periodic_clear;
  }

  void add(time_ms_t interval, std::function<void()> clear) {
    const std::lock_guard<std::mutex> lock(mutex);
    std::vector<std::function<void()>> &clears = clears_by_interval[interval];
    clears.push_back(std::move(clear));
    if (clears.size() > 1) {
      return;
    }
    std::thread([this, interval]() {
      while (!cfg.quitting) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        if (cfg.quitting) {
          break;
        }
        tick(interval);
      }
    }).detach();
  }
};

class VectorRegister : public SynapseDS {
private:
  std::vector<Register> registers;
  size_t capacity;
  bits_t value_size;
  const time_ms_t periodic_clear_interval;

public:
  VectorRegister(const std::string &_name, const std::vector<std::string> &register_names, time_ms_t _periodic_clear_interval = 0)
      : SynapseDS(_name), capacity(0), value_size(0), periodic_clear_interval(_periodic_clear_interval) {
    assert(!register_names.empty() && "Register names must not be empty");

    for (const std::string &name : register_names) {
      registers.emplace_back(name);
      capacity = registers.back().get_capacity();
      value_size += registers.back().get_value_size() * (registers.back().is_paired() ? 2 : 1);
    }

    for (const Register &reg : registers) {
      assert(reg.get_capacity() == capacity);
    }

    if (periodic_clear_interval > 0) {
      PeriodicClear::instance().add(periodic_clear_interval, [this]() { clear(); });
    }
  }

  void clear() {
    for (Register &reg : registers) {
      reg.reset_all_entries();
    }
  }

  // Read from the switch: the data plane updates the cells too, so what the controller last wrote
  // is no answer. The pipe carrying the traffic holds the live value, the others what the
  // controller wrote (its writes go to every pipe), as with the other register-backed structures.
  void get(u32 index, buffer_t &v) {
    assert(index < capacity);
    v = buffer_t(value_size / 8);

    bytes_t offset = 0;
    for (Register &reg : registers) {
      const bytes_t reg_value_size = reg.get_value_size() / 8;
      if (reg.is_paired()) {
        const auto [lo, hi] = reg.get_pair_max(index);
        v.set(offset, reg_value_size, lo);
        v.set(offset + reg_value_size, reg_value_size, hi);
        offset += 2 * reg_value_size;
        continue;
      }
      v.set(offset, reg_value_size, reg.get_max(index));
      offset += reg_value_size;
    }
  }

  // The cell as `pipe` holds it: the pipe a packet came through has the data plane's latest
  // value of its cell, while the others may hold what the controller last wrote, which the max
  // over the pipes would mistake for the live one once the data plane swapped in a lighter cell.
  void get(u32 index, buffer_t &v, u16 pipe) {
    assert(index < capacity);
    v = buffer_t(value_size / 8);

    bytes_t offset = 0;
    for (Register &reg : registers) {
      const bytes_t reg_value_size = reg.get_value_size() / 8;
      if (reg.is_paired()) {
        const auto [lo, hi] = reg.get_pair(index, pipe);
        v.set(offset, reg_value_size, lo);
        v.set(offset + reg_value_size, reg_value_size, hi);
        offset += 2 * reg_value_size;
        continue;
      }
      v.set(offset, reg_value_size, reg.get(index, pipe));
      offset += reg_value_size;
    }
  }

  void put(u32 index, const buffer_t &v) {
    assert(index < capacity);
    assert(v.size == value_size / 8);

    bytes_t offset = 0;
    for (Register &reg : registers) {
      const bytes_t reg_value_size = reg.get_value_size() / 8;
      if (reg.is_paired()) {
        reg.set_pair(index, v.get(offset, reg_value_size), v.get(offset + reg_value_size, reg_value_size));
        offset += 2 * reg_value_size;
        continue;
      }
      const u32 value = v.get(offset, reg_value_size);
      reg.set(index, value);

      offset += reg_value_size;
    }
  }

  void dump() const {
    std::stringstream ss;
    dump(ss);
    LOG("%s", ss.str().c_str());
  }

  void dump(std::ostream &os) const {
    for (const Register &reg : registers) {
      reg.dump(os);
    }
  }
};

} // namespace sycon