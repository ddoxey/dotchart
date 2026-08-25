#include "options.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>

#if defined(__unix__) || defined(__APPLE__)
#include <getopt.h>
#else
// If you want Windows later: consider cxxopts or argparse libraries.
#endif

namespace dotchart {

static bool looks_like_format_arg(const char* s) {
  if (!s || *s == '\0') return false;
  if (s[0] == '-') return false;
  return std::strchr(s, '%') != nullptr;
}

static bool parse_int(const char* s, int& out) {
  char* end = nullptr;
  long v = std::strtol(s, &end, 10);
  if (!s || *s == '\0' || (end && *end != '\0')) return false;
  if (v < std::numeric_limits<int>::min() ||
      v > std::numeric_limits<int>::max())
    return false;
  out = static_cast<int>(v);
  return true;
}

static bool parse_double(const char* s, double& out) {
  char* end = nullptr;
  double v = std::strtod(s, &end);
  if (!s || *s == '\0' || (end && *end != '\0')) return false;
  out = v;
  return true;
}

static bool looks_like_color_arg(const char* s) {
  if (!s || *s == '\0') return false;
  if (s[0] == '-') return false;
  return (std::strstr(s, "..") != nullptr) || (std::strchr(s, ',') != nullptr);
}

static bool parse_color_spec(const std::string& spec, std::vector<int>& out,
                             std::string& err, const char* opt_name,
                             int max_code) {
  out.clear();
  const int hi = std::max(0, max_code);
  const std::string range_example = (hi <= 15) ? "1..14" : "196..231";
  const std::string list_example = (hi <= 15) ? "1,3,5" : "160,167,174";
  if (spec.find("..") != std::string::npos) {
    size_t pos = spec.find("..");
    if (spec.find("..", pos + 2) != std::string::npos) {
      err = "'" + std::string(opt_name) +
            "' range expects a single \"a..b\" pair.";
      return false;
    }
    std::string a_str = spec.substr(0, pos);
    std::string b_str = spec.substr(pos + 2);
    int a = 0;
    int b = 0;
    if (!parse_int(a_str.c_str(), a) || !parse_int(b_str.c_str(), b)) {
      err = "'" + std::string(opt_name) + "' range expects integers (e.g. " +
            range_example + ").";
      return false;
    }
    if (a < 0 || a > hi || b < 0 || b > hi) {
      err = "'" + std::string(opt_name) + "' range must be within 0.." +
            std::to_string(hi) + ".";
      return false;
    }
    int step = (a <= b) ? 1 : -1;
    for (int v = a; v != b; v += step) out.push_back(v);
    out.push_back(b);
    return true;
  }

  size_t start = 0;
  while (start < spec.size()) {
    size_t comma = spec.find(',', start);
    std::string token = spec.substr(
        start, comma == std::string::npos ? std::string::npos : comma - start);
    int v = 0;
    if (!parse_int(token.c_str(), v)) {
      err = "'" + std::string(opt_name) + "' list expects integers (e.g. " +
            list_example + ").";
      return false;
    }
    if (v < 0 || v > hi) {
      err = "'" + std::string(opt_name) + "' list values must be within 0.." +
            std::to_string(hi) + ".";
      return false;
    }
    out.push_back(v);
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  if (out.size() < 2) {
    err = "'" + std::string(opt_name) + "' list expects at least two values.";
    return false;
  }
  return true;
}

std::string version_text() { return "dotchart 0.1.0"; }

std::string help_text() {
  // Keep this short; you can expand as features land.
  return R"(Usage: dotchart [OPTIONS] [FILE]

Reads numeric values (CSV or whitespace) and prints a Braille-based terminal chart.

Input:
  -F, --field-sep SEP      Field separator (default: auto: commas+whitespace)
  -f, --file FILE          Read from FILE (default: stdin)
  -c, --column N           Use column N as y values (enables column mode)
      --x-column N         Use column N as x labels (enables column mode)

Layout:
  -W, --width COLS|PCT%   Output width (default: fit-to-data; capped by tty width)
  -H, --height ROWS        Output height in braille rows (default: 9)
  -T, --title TEXT         Title printed above the chart

Scaling:
  -M, --max VALUE          Force max scale (default: auto)
  -m, --min VALUE          Force min scale (default: auto)
  -S, --signed             Force signed chart (baseline at 0)

Labels:
  -x, --x-axis[=FMT]       Show x-axis labels (default format: "%d")
      --x-min-axis[=FMT]   Mark and label local minima (default format: "%d")
  -y, --y-axis[=FMT]       Show y-axis labels (default format: "%3.0f ")

Other:
      --no-unicode         ASCII fallback (placeholder)
      --style=bar|point    Render as bars (default) or points
      --color[=SPEC]       Enable ANSI color output (auto-detect)
                           SPEC: "a..b" or "a,b,c" for 256-color ramp
                           Default ramps: positive and negative use different ranges
      --color-pos[=SPEC]   Set positive-side ramp (mode: auto if no SPEC)
      --color-neg[=SPEC]   Set negative-side ramp (mode: auto if no SPEC)
      --16-color           Force ANSI 16-color output
      --16-color-pos[=SPEC]
      --16-color-neg[=SPEC]
      --256-color          Force ANSI 256-color output
      --256-color-pos[=SPEC]
      --256-color-neg[=SPEC]
                           Side-specific ramps override generic --color ramp
  -h, --help               Show help
  -v, --version            Show version
  -d, --debug              Show debugging data

Examples:
  echo "1,10,20,10,1" | dotchart -F,
  dotchart -W 120 -H 8 -M 100 -y "$%+4.0f " values.csv
)";
}

ParseResult parse_args(int argc, char** argv) {
  ParseResult r;

#if defined(__unix__) || defined(__APPLE__)
  const char* short_opts = "F:f:c:W:H:T:M:m:Sx::y::hvd";
  static option long_opts[] = {{"field-sep", required_argument, nullptr, 'F'},
                               {"file", required_argument, nullptr, 'f'},
                               {"column", required_argument, nullptr, 'c'},
                               {"x-column", required_argument, nullptr, 1012},
                               {"width", required_argument, nullptr, 'W'},
                               {"height", required_argument, nullptr, 'H'},
                               {"title", required_argument, nullptr, 'T'},
                               {"max", required_argument, nullptr, 'M'},
                               {"min", required_argument, nullptr, 'm'},
                               {"signed", no_argument, nullptr, 'S'},
                               {"x-axis", optional_argument, nullptr, 'x'},
                               {"x-min-axis", optional_argument, nullptr,
                                1011},
                               {"y-axis", optional_argument, nullptr, 'y'},
                               {"no-unicode", no_argument, nullptr, 1000},
                               {"style", required_argument, nullptr, 1004},
                               {"color", optional_argument, nullptr, 1001},
                               {"color-pos", optional_argument, nullptr, 1005},
                               {"color-neg", optional_argument, nullptr, 1006},
                               {"16-color", no_argument, nullptr, 1002},
                               {"16-color-pos", optional_argument, nullptr,
                                1007},
                               {"16-color-neg", optional_argument, nullptr,
                                1008},
                               {"256-color", no_argument, nullptr, 1003},
                               {"256-color-pos", optional_argument, nullptr,
                                1009},
                               {"256-color-neg", optional_argument, nullptr,
                                1010},
                               {"help", no_argument, nullptr, 'h'},
                               {"version", no_argument, nullptr, 'v'},
                               {"debug", no_argument, nullptr, 'd'},
                               {nullptr, 0, nullptr, 0}};

  // Reset getopt state in case parse_args is called multiple times in tests.
  optind = 1;

  auto parse_color_opt = [&](const char* flag_name, int max_code,
                             std::vector<int>& target_ramp) {
    auto parse_one = [&](const char* spec_arg) {
      std::string err;
      if (!parse_color_spec(spec_arg, target_ramp, err, flag_name, max_code)) {
        r.errors.push_back(err);
        return false;
      }
      return true;
    };

    if (optarg && *optarg) return parse_one(optarg);
    if (optind < argc && looks_like_color_arg(argv[optind])) {
      if (parse_one(argv[optind])) {
        ++optind;
        return true;
      }
      return false;
    }
    return false;
  };

  while (true) {
    int long_index = 0;
    int c = getopt_long(argc, argv, short_opts, long_opts, &long_index);
    if (c == -1) break;

    switch (c) {
      case 'F': {
        // v0: single-char separator. Accept escape-ish "\t" and "\n".
        std::string s = optarg ? optarg : "";
        if (s == "\\t")
          r.opts.field_sep = '\t';
        else if (s == "\\n")
          r.opts.field_sep = '\n';
        else if (s.size() == 1)
          r.opts.field_sep = s[0];
        else if (s.empty())
          r.opts.field_sep = '\0';
        else
          r.errors.push_back(
              "'-F/--field-sep' expects a single character (or \\t, \\n).");
        break;
      }
      case 'f':
        r.opts.file = std::string(optarg);
        break;
      case 'c': {
        int n = 0;
        if (!parse_int(optarg, n) || n <= 0)
          r.errors.push_back("'-c/--column' expects a positive integer.");
        else
          r.opts.column = n;
        break;
      }
      case 1012: {
        int n = 0;
        if (!parse_int(optarg, n) || n <= 0)
          r.errors.push_back("'--x-column' expects a positive integer.");
        else
          r.opts.x_column = n;
        break;
      }
      case 'W': {
        std::string s = optarg ? optarg : "";
        if (s.empty()) {
          r.errors.push_back("'-W/--width' expects a value (e.g. 80 or 80%).");
          break;
        }

        if (!s.empty() && s.back() == '%') {
          s.pop_back();
          int p = 0;
          if (!parse_int(s.c_str(), p) || p <= 0 || p > 100) {
            r.errors.push_back(
                "'-W/--width' percent must be in 1..100 (e.g. 80%).");
          } else {
            r.opts.width.kind = WidthSpec::Kind::Percent;
            r.opts.width.percent = p;
          }
        } else {
          int w = 0;
          if (!parse_int(s.c_str(), w) || w <= 0) {
            r.errors.push_back(
                "'-W/--width' expects a positive integer (e.g. 80) or percent "
                "(e.g. 80%).");
          } else {
            r.opts.width.kind = WidthSpec::Kind::Cols;
            r.opts.width.cols = w;
          }
        }
        break;
      }
      case 'H': {
        int h = 0;
        if (!parse_int(optarg, h) || h <= 0)
          r.errors.push_back("'-H/--height' expects a positive integer.");
        else
          r.opts.height = h;
        break;
      }
      case 'T':
        r.opts.title = std::string(optarg);
        break;
      case 'M': {
        double v = 0;
        if (!parse_double(optarg, v))
          r.errors.push_back("'-M/--max' expects a number.");
        else
          r.opts.max_value = v;
        break;
      }
      case 'm': {
        double v = 0;
        if (!parse_double(optarg, v))
          r.errors.push_back("'-m/--min' expects a number.");
        else
          r.opts.min_value = v;
        break;
      }
      case 'S':
        r.opts.force_signed = true;
        break;
      case 'x':
        r.opts.show_x_axis = true;
        if (optarg && *optarg) {
          r.opts.x_axis_fmt = std::string(optarg);
        } else if (optind < argc && looks_like_format_arg(argv[optind])) {
          r.opts.x_axis_fmt = std::string(argv[optind]);
          ++optind;
        }
        break;
      case 1011:
        r.opts.show_x_min_axis = true;
        if (optarg && *optarg) {
          r.opts.x_min_axis_fmt = std::string(optarg);
        } else if (optind < argc && looks_like_format_arg(argv[optind])) {
          r.opts.x_min_axis_fmt = std::string(argv[optind]);
          ++optind;
        }
        break;
      case 'y':
        r.opts.show_y_axis = true;
        if (optarg && *optarg) {
          r.opts.y_axis_fmt = std::string(optarg);
          r.opts.y_axis_fmt_explicit = true;
        } else if (optind < argc && looks_like_format_arg(argv[optind])) {
          r.opts.y_axis_fmt = std::string(argv[optind]);
          r.opts.y_axis_fmt_explicit = true;
          ++optind;
        }
        break;
      case 1000:
        r.opts.unicode = false;
        break;
      case 1004: {
        std::string s = optarg ? optarg : "";
        if (s == "bar") {
          r.opts.style = Options::ChartStyle::Bar;
        } else if (s == "point") {
          r.opts.style = Options::ChartStyle::Point;
        } else {
          r.errors.push_back("'--style' expects 'bar' or 'point'.");
        }
        break;
      }
      case 1001:
        r.opts.color_mode = Options::ColorMode::Auto;
        if (parse_color_opt("--color", 255, r.opts.color_ramp)) {
          r.opts.color_mode = Options::ColorMode::Ansi256;
        }
        break;
      case 1005:
        r.opts.color_mode = Options::ColorMode::Auto;
        if (parse_color_opt("--color-pos", 255, r.opts.color_ramp_pos)) {
          r.opts.color_mode = Options::ColorMode::Ansi256;
        }
        break;
      case 1006:
        r.opts.color_mode = Options::ColorMode::Auto;
        if (parse_color_opt("--color-neg", 255, r.opts.color_ramp_neg)) {
          r.opts.color_mode = Options::ColorMode::Ansi256;
        }
        break;
      case 1002:
        r.opts.color_mode = Options::ColorMode::Ansi16;
        break;
      case 1007:
        r.opts.color_mode = Options::ColorMode::Ansi16;
        parse_color_opt("--16-color-pos", 15, r.opts.color_ramp_pos);
        break;
      case 1008:
        r.opts.color_mode = Options::ColorMode::Ansi16;
        parse_color_opt("--16-color-neg", 15, r.opts.color_ramp_neg);
        break;
      case 1003:
        r.opts.color_mode = Options::ColorMode::Ansi256;
        break;
      case 1009:
        r.opts.color_mode = Options::ColorMode::Ansi256;
        parse_color_opt("--256-color-pos", 255, r.opts.color_ramp_pos);
        break;
      case 1010:
        r.opts.color_mode = Options::ColorMode::Ansi256;
        parse_color_opt("--256-color-neg", 255, r.opts.color_ramp_neg);
        break;
      case 'h':
        r.opts.help = true;
        break;
      case 'v':
        r.opts.version = true;
        break;
      case 'd':
        r.opts.debug = true;
        break;
      default:
        r.errors.push_back("Unknown option. Use --help.");
        break;
    }
  }

  // Positional FILE like awk: dotchart [opts] file
  if (!r.opts.file && optind < argc) {
    r.opts.file = std::string(argv[optind]);
    optind++;
  }
  if (optind < argc) {
    r.errors.push_back("Too many positional arguments. Use at most one FILE.");
  }
  if (r.opts.show_x_axis && r.opts.show_x_min_axis) {
    r.errors.push_back("'--x-axis' and '--x-min-axis' are mutually exclusive.");
  }
#else
  r.errors.push_back("Argument parsing not implemented on this platform yet.");
#endif

  return r;
}

}  // namespace dotchart
