#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace LibCore {

inline std::string find_and_replace(const std::string &str, const std::vector<std::pair<std::string, std::string>> &replacements) {
  std::string result = str;
  for (const std::pair<std::string, std::string> &replacement : replacements) {
    const std::string &before = replacement.first;
    const std::string &after  = replacement.second;

    std::string::size_type n = 0;
    while ((n = result.find(before, n)) != std::string::npos) {
      result.replace(n, before.size(), after);
      n += after.size();
    }
  }
  return result;
}

inline std::string sanitize_html_label(const std::string &label) {
  return find_and_replace(label, {
                                     {"&", "&amp;"}, // Careful, this needs to be the first
                                                     // one, otherwise we mess everything up. Notice
                                                     // that all the others use ampersands.
                                     {"{", "&#123;"},
                                     {"}", "&#125;"},
                                     {"[", "&#91;"},
                                     {"]", "&#93;"},
                                     {"<", "&lt;"},
                                     {">", "&gt;"},
                                     {"\n", "<br/>"},
                                 });
}

// A comparison function that implements "natural order" comparison for strings.
// That is, it treats sequences of digits as numerical values rather than
// individual characters. For example, "file2" will be considered less than
// "file10".
inline bool natural_compare(const std::string &a, const std::string &b) {
  size_t i = 0, j = 0;

  while (i < a.size() && j < b.size()) {
    if (std::isdigit(a[i]) && std::isdigit(b[j])) {
      const size_t start_i = i, start_j = j;
      while (i < a.size() && std::isdigit(a[i]))
        ++i;
      while (j < b.size() && std::isdigit(b[j]))
        ++j;

      // Compared as digit strings (shorter is smaller, then lexicographic), which is numeric
      // order for any length: object addresses in names overflow an int.
      std::string_view num_a(a.data() + start_i, i - start_i), num_b(b.data() + start_j, j - start_j);
      num_a.remove_prefix(std::min(num_a.find_first_not_of('0'), num_a.size()));
      num_b.remove_prefix(std::min(num_b.find_first_not_of('0'), num_b.size()));

      if (num_a != num_b)
        return num_a.size() != num_b.size() ? num_a.size() < num_b.size() : num_a < num_b;
    } else {
      if (a[i] != b[j])
        return a[i] < b[j];
      ++i;
      ++j;
    }
  }

  return a.size() < b.size();
}

} // namespace LibCore