#include "input.h"

#include <algorithm>
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

static std::string trim(std::string value) {
  size_t first = 0;
  while (first < value.size() && is_ws(value[first])) ++first;
  size_t last = value.size();
  while (last > first && is_ws(value[last - 1])) --last;
  return value.substr(first, last - first);
}

static bool parse_double_token(const std::string& token, double& value) {
  if (token.empty()) return false;
#if defined(__cpp_lib_to_chars) && (__cpp_lib_to_chars >= 201611L)
  auto res = std::from_chars(token.data(), token.data() + token.size(), value);
  if (res.ec == std::errc{} && res.ptr == token.data() + token.size()) {
    return true;
  }
#endif
  char* end = nullptr;
  value = std::strtod(token.c_str(), &end);
  return end && end != token.c_str() && *end == '\0';
}

static void capture_first_token_meta(ParsedValues& parsed,
                                     const std::string& token) {
  if (parsed.has_first_token) return;
  parsed.has_first_token = true;
  parsed.first_has_exp = (token.find('e') != std::string::npos) ||
                         (token.find('E') != std::string::npos);
  const size_t dot = token.find('.');
  if (dot == std::string::npos || parsed.first_has_exp) {
    parsed.first_is_int = !parsed.first_has_exp;
    parsed.first_precision = 0;
    return;
  }
  parsed.first_is_int = false;
  size_t end = token.find_first_of("eE", dot + 1);
  if (end == std::string::npos) end = token.size();
  parsed.first_precision =
      (end > dot + 1) ? static_cast<int>(end - (dot + 1)) : 0;
}

static std::vector<std::string> split_record(const Options& opts,
                                             const std::string& line) {
  std::vector<std::string> fields;
  if (opts.field_sep != '\0') {
    size_t start = 0;
    while (true) {
      const size_t end = line.find(opts.field_sep, start);
      fields.push_back(trim(line.substr(
          start, end == std::string::npos ? std::string::npos : end - start)));
      if (end == std::string::npos) break;
      start = end + 1;
    }
    return fields;
  }

  size_t i = 0;
  while (i < line.size()) {
    while (i < line.size() && (line[i] == ',' || is_ws(line[i]))) ++i;
    if (i >= line.size()) break;
    size_t end = i;
    while (end < line.size() && line[end] != ',' && !is_ws(line[end])) ++end;
    fields.push_back(line.substr(i, end - i));
    i = end;
  }
  return fields;
}

static ParsedValues parse_column_records(const Options& opts,
                                         const std::string& text,
                                         bool capture_meta) {
  ParsedValues parsed;
  const int x_column = opts.x_column.value_or(1);
  const int y_column = opts.column.value_or(2);
  const size_t required = static_cast<size_t>(std::max(x_column, y_column));

  std::istringstream lines(text);
  std::string line;
  while (std::getline(lines, line)) {
    const auto fields = split_record(opts, line);
    if (fields.size() < required) continue;
    const std::string& x_label = fields[static_cast<size_t>(x_column - 1)];
    const std::string& y_token = fields[static_cast<size_t>(y_column - 1)];
    double value = 0.0;
    if (x_label.empty() || !parse_double_token(y_token, value)) continue;
    parsed.x_labels.push_back(x_label);
    parsed.values.push_back(value);
    if (capture_meta) capture_first_token_meta(parsed, y_token);
  }
  return parsed;
}

static ParsedValues parse_values_inner(const Options& opts,
                                       const std::string& text,
                                       bool capture_meta) {
  if (opts.column || opts.x_column) {
    return parse_column_records(opts, text, capture_meta);
  }

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
    const std::string token(s + i, s + j);
    const bool parsed_value = parse_double_token(token, v);

    if (parsed_value) {
      parsed.values.push_back(v);
      if (capture_meta && !parsed.has_first_token) {
        capture_first_token_meta(parsed, token);
      }
    }

    i = j;
  }

  return parsed;
}

std::vector<double> parse_values(const Options& opts, const std::string& text) {
  return parse_values_inner(opts, text, false).values;
}

ParsedValues parse_values_with_meta(const Options& opts,
                                    const std::string& text) {
  return parse_values_inner(opts, text, true);
}

}  // namespace dotchart
