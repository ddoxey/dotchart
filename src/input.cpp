#include "input.h"

#include <cctype>
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>

namespace dotchart {

std::string read_all_input(const Options& opts) {
  std::ostringstream oss;
  if (opts.file) {
    std::ifstream in(*opts.file, std::ios::binary);
    if (!in) {
      throw std::runtime_error("Failed to open file: " + *opts.file);
    }
    oss << in.rdbuf();
  } else {
    oss << std::cin.rdbuf();
  }
  return oss.str();
}

static inline bool is_ws(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

std::vector<double> parse_values(const Options& opts, const std::string& text) {
  std::vector<double> out;
  out.reserve(1024);

  auto is_delim = [&](char ch) -> bool {
    if (opts.field_sep != '\0') return ch == opts.field_sep;
    // auto: commas or whitespace
    return ch == ',' || is_ws(ch);
  };

  // Tokenize without allocations: scan runs of non-delims.
  const char* s = text.data();
  size_t n = text.size();
  size_t i = 0;

  while (i < n) {
    while (i < n && is_delim(s[i])) i++;
    if (i >= n) break;
    size_t j = i;
    while (j < n && !is_delim(s[j])) j++;

    // parse [i, j)
    double v = 0.0;
    // from_chars for float is C++17 but widely supported; if your libstdc++ is old,
    // fall back to strtod.
#if defined(__cpp_lib_to_chars) && (__cpp_lib_to_chars >= 201611L)
    auto res = std::from_chars(s + i, s + j, v);
    if (res.ec == std::errc{} && res.ptr == s + j) {
      out.push_back(v);
    } else {
      // fallback: try strtod for cases like "1e-3"
      char* end = nullptr;
      std::string tmp(s + i, s + j);
      v = std::strtod(tmp.c_str(), &end);
      if (end && *end == '\0') out.push_back(v);
    }
#else
    char* end = nullptr;
    std::string tmp(s + i, s + j);
    v = std::strtod(tmp.c_str(), &end);
    if (end && *end == '\0') out.push_back(v);
#endif

    i = j;
  }

  return out;
}

} // namespace dotchart
