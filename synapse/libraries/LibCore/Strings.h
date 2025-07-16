#pragma once

#include <string>
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

} // namespace LibCore