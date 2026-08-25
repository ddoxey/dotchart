#pragma once
#include <string>
#include <vector>

#include "options.h"

namespace dotchart {

// Read entire input from file or stdin into a string.
std::string read_all_input(const Options& opts);

struct ParsedValues {
  std::vector<double> values;
  std::vector<std::string> x_labels;
  bool has_first_token = false;
  bool first_is_int = false;
  bool first_has_exp = false;
  int first_precision = 0;
};

// Parse doubles from a legacy flat stream, or records when either column
// option is set. Column mode defaults to x column 1 and y column 2.
std::vector<double> parse_values(const Options& opts, const std::string& text);
ParsedValues parse_values_with_meta(const Options& opts,
                                    const std::string& text);

}  // namespace dotchart
