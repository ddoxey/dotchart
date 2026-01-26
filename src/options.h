#pragma once
#include <optional>
#include <string>
#include <vector>

namespace dotchart {

struct WidthSpec {
  enum class Kind { Auto, Cols, Percent };
  Kind kind = Kind::Auto;
  int cols = 0;
  int percent = 0;
};

struct Options {
  // Input
  char field_sep = '\0';                 // '\0' => auto (comma + whitespace)
  std::optional<std::string> file;       // if unset, read stdin
  std::optional<int> column;             // 1-based column selection (optional; future)

  // Layout
  WidthSpec width;  // columns
  int height = 10;  // braille cell rows (each is 4 dot rows)
  std::optional<std::string> title;

  // Scaling
  std::optional<double> min_value;       // forced min (optional)
  std::optional<double> max_value;       // forced max (optional)
  bool force_signed = false;             // show baseline even if no negatives

  // Labels
  bool show_y_axis = false;
  std::string y_axis_fmt = "%3.0f ";     // printf-style

  // Output
  bool unicode = true;

  // Misc
  bool help = false;
  bool version = false;
  bool debug = false;
};

struct ParseResult {
  Options opts;
  std::vector<std::string> errors;
};

ParseResult parse_args(int argc, char** argv);
std::string help_text();
std::string version_text();

} // namespace dotchart
