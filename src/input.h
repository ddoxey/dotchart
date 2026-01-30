#pragma once
#include "options.h"
#include <string>
#include <vector>

namespace dotchart {

// Read entire input from file or stdin into a string.
std::string read_all_input(const Options& opts);

struct ParsedValues {
  std::vector<double> values;
  bool has_first_token = false;
  bool first_is_int = false;
  bool first_has_exp = false;
  int first_precision = 0;
};

// Parse doubles from input, supporting comma/whitespace delimiters.
// v0 ignores columns; future can support column selection.
std::vector<double> parse_values(const Options& opts, const std::string& text);
ParsedValues parse_values_with_meta(const Options& opts, const std::string& text);

} // namespace dotchart
