#pragma once

#include <array>
#include <optional>
#include <vector>

#include "synapse_ds.h"
#include "../constants.h"
#include "../primitives/register.h"
#include "../time.h"
#include "../buffer.h"

namespace sycon {

class VectorRegister : public SynapseDS {
private:
  std::vector<buffer_t> cache;
  std::vector<Register> registers;
  size_t capacity;
  bits_t value_size;

public:
  VectorRegister(const std::string &_name, const std::vector<std::string> &register_names) : SynapseDS(_name), capacity(0), value_size(0) {
    assert(!register_names.empty() && "Register names must not be empty");

    for (const std::string &name : register_names) {
      registers.emplace_back(name);
      capacity = registers.back().get_capacity();
      value_size += registers.back().get_value_size();
    }

    for (const Register &reg : registers) {
      assert(reg.get_capacity() == capacity);
    }

    cache.assign(capacity, buffer_t(value_size / 8));
  }

  void get(u32 index, buffer_t &v) const {
    assert(index < capacity);
    v = cache.at(index);
  }

  void put(u32 index, const buffer_t &v) {
    assert(index < capacity);
    assert(v.size == value_size / 8);

    bytes_t offset = 0;
    for (Register &reg : registers) {
      const bytes_t reg_value_size = reg.get_value_size() / 8;
      const u32 value              = v.get(offset, reg_value_size);
      reg.set(index, value);

      offset += reg_value_size;
    }

    cache.at(index) = v;
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