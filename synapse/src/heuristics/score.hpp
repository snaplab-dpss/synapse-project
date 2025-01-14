#pragma once

#include <iostream>
#include <vector>

#include "../execution_plan/execution_plan.hpp"
#include "../execution_plan/node.hpp"
#include "../random_engine.hpp"
#include "../system.hpp"

namespace synapse {
struct Score {
  std::vector<i64> values;

public:
  Score(const std::vector<i64> &_values) : values(_values) {}
  Score(const Score &score) : values(score.values) {}

  inline bool operator<(const Score &other) {
    assert(values.size() == other.values.size() && "Scores have different sizes");
    for (size_t i = 0; i < values.size(); i++) {
      i64 this_score  = values[i];
      i64 other_score = other.values[i];

      if (this_score > other_score) {
        return false;
      }

      if (this_score < other_score) {
        return true;
      }
    }

    return false;
  }

  inline bool operator==(const Score &other) {
    assert(values.size() == other.values.size() && "Scores have different sizes");
    for (size_t i = 0; i < values.size(); i++) {
      i64 this_score  = values[i];
      i64 other_score = other.values[i];

      if (this_score != other_score) {
        return false;
      }
    }

    return true;
  }

  inline bool operator>(const Score &other) { return !((*this) < other) && !((*this) == other); }

  inline bool operator<=(const Score &other) { return !((*this) > other); }
  inline bool operator>=(const Score &other) { return !((*this) < other); }
  inline bool operator!=(const Score &other) { return !((*this) == other); }
};

inline std::ostream &operator<<(std::ostream &os, const Score &score) {
  size_t N = score.values.size();

  os << "<";

  bool first = true;
  for (size_t i = 0u; i < N; i++) {
    if (!first)
      os << ",";
    first = false;
    os << int2hr(score.values[i]);
  }

  os << ">";
  return os;
}
} // namespace synapse