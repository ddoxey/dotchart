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

static ParsedValues parse_values_inner(const Options& opts, const std::string& text, bool capture_meta) {
  ParsedValues parsed;
  parsed.values.reserve(1024);

  auto is_delim = [&](char ch) -> bool {
    if (opts.field_sep != '\0') {
      // Explicit separator still treats surrounding whitespace as delimiters.
      return ch == opts.field_sep || is_ws(ch);
    }
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
    bool parsed_value = false;
    // from_chars for float is C++17 but widely supported; if your libstdc++ is old,
    // fall back to strtod.
#if defined(__cpp_lib_to_chars) && (__cpp_lib_to_chars >= 201611L)
    auto res = std::from_chars(s + i, s + j, v);
    if (res.ec == std::errc{} && res.ptr == s + j) {
      parsed_value = true;
    } else {
      // fallback: try strtod for cases like "1e-3"
      char* end = nullptr;
      std::string tmp(s + i, s + j);
      v = std::strtod(tmp.c_str(), &end);
      if (end && *end == '\0') parsed_value = true;
    }
#else
    char* end = nullptr;
    std::string tmp(s + i, s + j);
    v = std::strtod(tmp.c_str(), &end);
    if (end && *end == '\0') parsed_value = true;
#endif

    if (parsed_value) {
      parsed.values.push_back(v);
      if (capture_meta && !parsed.has_first_token) {
        std::string tok(s + i, s + j);
        parsed.has_first_token = true;
        parsed.first_has_exp = (tok.find('e') != std::string::npos) ||
                               (tok.find('E') != std::string::npos);
        size_t dot = tok.find('.');
        if (dot == std::string::npos || parsed.first_has_exp) {
          parsed.first_is_int = !parsed.first_has_exp;
          parsed.first_precision = 0;
        } else {
          parsed.first_is_int = false;
          size_t end = tok.find_first_of("eE", dot + 1);
          if (end == std::string::npos) end = tok.size();
          if (end > dot + 1) {
            parsed.first_precision = static_cast<int>(end - (dot + 1));
          } else {
            parsed.first_precision = 0;
          }
        }
      }
    }

    i = j;
  }

  return parsed;
}

std::vector<double> parse_values(const Options& opts, const std::string& text) {
  return parse_values_inner(opts, text, false).values;
}

ParsedValues parse_values_with_meta(const Options& opts, const std::string& text) {
  return parse_values_inner(opts, text, true);
}

} // namespace dotchart
