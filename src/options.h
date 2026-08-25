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
  enum class ChartStyle { Bar, Point };

  // Input
  char field_sep = '\0';            // '\0' => auto (comma + whitespace)
  std::optional<std::string> file;  // if unset, read stdin
  std::optional<int> column;    // 1-based y-value column
  std::optional<int> x_column;  // 1-based x-label column

  // Layout
  WidthSpec width;  // columns
  int height = 9;   // braille cell rows (each is 4 dot rows)
  std::optional<std::string> title;

  // Scaling
  std::optional<double> min_value;  // forced min (optional)
  std::optional<double> max_value;  // forced max (optional)
  bool force_signed = false;        // show baseline even if no negatives

  // Labels
  bool show_y_axis = false;
  std::string y_axis_fmt = "%3.0f ";  // printf-style
  bool y_axis_fmt_explicit = false;
  bool y_axis_fmt_is_int = false;
  bool show_x_axis = false;
  std::string x_axis_fmt = "%d";  // printf-style (index labels)
  bool show_x_min_axis = false;    // label strict local minima only
  std::string x_min_axis_fmt = "%d";

  // Output
  bool unicode = true;
  ChartStyle style = ChartStyle::Bar;
  enum class ColorMode { Off, Auto, Ansi16, Ansi256 };
  ColorMode color_mode = ColorMode::Off;
  std::vector<int> color_ramp;
  std::vector<int> color_ramp_pos;
  std::vector<int> color_ramp_neg;

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

}  // namespace dotchart
