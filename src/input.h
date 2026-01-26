#pragma once
#include "options.h"
#include <string>
#include <vector>

namespace dotchart {

// Read entire input from file or stdin into a string.
std::string read_all_input(const Options& opts);

// Parse doubles from input, supporting comma/whitespace delimiters.
// v0 ignores columns; future can support column selection.
std::vector<double> parse_values(const Options& opts, const std::string& text);

} // namespace dotchart
