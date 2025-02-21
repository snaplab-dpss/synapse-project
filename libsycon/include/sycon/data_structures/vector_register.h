#pragma once

#include <array>
#include <optional>
#include <vector>

#include "../constants.h"
#include "../primitives/register.h"
#include "../time.h"
#include "../buffer.h"

namespace sycon {

class VectorRegister {
private:
  std::vector<buffer_t> cache;
  std::vector<PrimitiveRegister> registers;
  size_t capacity;
  bits_t value_size;

public:
  VectorRegister(const std::string &_control_register_name, const std::vector<std::string> &_register_names) : capacity(0), value_size(0) {
    assert(!_register_names.empty() && "Register names must not be empty");

    for (const std::string &name : _register_names) {
      registers.emplace_back(_control_register_name, name);
      capacity = registers.back().get_capacity();
      value_size += registers.back().get_value_size();
    }

    for (const PrimitiveRegister &reg : registers) {
      assert(reg.get_capacity() == capacity);
    }

    cache.assign(capacity, buffer_t(value_size / 8));
  }

  void get(u32 index, buffer_t &v) const {
    assert(index < capacity);
    v = cache[index];
  }

  void get_from_hw(u32 index, buffer_t &v) {
    assert(index < capacity);
    assert(false && "TODO");
  }

  void put(u32 index, const buffer_t &v) {
    assert(v.size == value_size / 8);
    cache[index] = v;

    bytes_t offset = 0;
    for (PrimitiveRegister &reg : registers) {
      bytes_t reg_value_size = reg.get_value_size() / 8;
      u32 value              = v.get_by_offset(offset, reg_value_size);
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
    for (const PrimitiveRegister &reg : registers) {
      os << "*********************** Register " << reg.get_name() << " ***********************\n";
      reg.dump(os);
    }
  }
};

} // namespace sycon