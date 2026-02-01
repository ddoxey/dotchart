#include "options.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#if defined(__unix__) || defined(__APPLE__)
  #include <getopt.h>
#else
  // If you want Windows later: consider cxxopts or argparse libraries.
#endif

namespace dotchart {

static bool parse_int(const char* s, int& out) {
  char* end = nullptr;
  long v = std::strtol(s, &end, 10);
  if (!s || *s == '\0' || (end && *end != '\0')) return false;
  if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max()) return false;
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

std::string version_text() {
  return "dotchart 0.1.0";
}

std::string help_text() {
  // Keep this short; you can expand as features land.
  return R"(Usage: dotchart [OPTIONS] [FILE]

Reads numeric values (CSV or whitespace) and prints a Braille-based terminal chart.

Input:
  -F, --field-sep SEP      Field separator (default: auto: commas+whitespace)
  -f, --file FILE          Read from FILE (default: stdin)
  -c, --column N           Use column N (1-based) from delimited input (reserved)

Layout:
  -W, --width COLS|PCT%   Output width (default: fit-to-data; capped by tty width)
  -H, --height ROWS        Output height in braille rows (default: 10)
  -T, --title TEXT         Title printed above the chart

Scaling:
  -M, --max VALUE          Force max scale (default: auto)
  -m, --min VALUE          Force min scale (default: auto)
  -S, --signed             Force signed chart (baseline at 0)

Labels:
  -x, --x-axis[=FMT]       Show x-axis labels (default format: "%d")
  -y, --y-axis[=FMT]       Show y-axis labels (default format: "%3.0f ")
  -Y, --y-fmt FMT          printf-style y label format (legacy)

Other:
      --no-unicode         ASCII fallback (placeholder)
      --color              Enable ANSI color output (auto-detect)
      --16-color           Force ANSI 16-color output
      --256-color          Force ANSI 256-color output
  -h, --help               Show help
  -v, --version            Show version
  -d, --debug              Show debugging data

Examples:
  echo "1,10,20,10,1" | dotchart -F,
  dotchart -W 120 -H 8 -M 100 -y -Y "$%+4.0f " values.csv
)";
}

ParseResult parse_args(int argc, char** argv) {
  ParseResult r;

#if defined(__unix__) || defined(__APPLE__)
  const char* short_opts = "F:f:c:W:H:T:M:m:SxyY:hvd";
  static option long_opts[] = {
    {"field-sep", required_argument, nullptr, 'F'},
    {"file", required_argument, nullptr, 'f'},
    {"column", required_argument, nullptr, 'c'},
    {"width", required_argument, nullptr, 'W'},
    {"height", required_argument, nullptr, 'H'},
    {"title", required_argument, nullptr, 'T'},
    {"max", required_argument, nullptr, 'M'},
    {"min", required_argument, nullptr, 'm'},
    {"signed", no_argument, nullptr, 'S'},
    {"x-axis", optional_argument, nullptr, 'x'},
    {"y-axis", optional_argument, nullptr, 'y'},
    {"y-fmt", required_argument, nullptr, 'Y'},
    {"no-unicode", no_argument, nullptr, 1000},
    {"color", no_argument, nullptr, 1001},
    {"16-color", no_argument, nullptr, 1002},
    {"256-color", no_argument, nullptr, 1003},
    {"help", no_argument, nullptr, 'h'},
    {"version", no_argument, nullptr, 'v'},
    {"debug", no_argument, nullptr, 'd'},
    {nullptr, 0, nullptr, 0}
  };

  // Reset getopt state in case parse_args is called multiple times in tests.
  optind = 1;

  while (true) {
    int long_index = 0;
    int c = getopt_long(argc, argv, short_opts, long_opts, &long_index);
    if (c == -1) break;

    switch (c) {
      case 'F': {
        // v0: single-char separator. Accept escape-ish "\t" and "\n".
        std::string s = optarg ? optarg : "";
        if (s == "\\t") r.opts.field_sep = '\t';
        else if (s == "\\n") r.opts.field_sep = '\n';
        else if (s.size() == 1) r.opts.field_sep = s[0];
        else if (s.empty()) r.opts.field_sep = '\0';
        else r.errors.push_back("'-F/--field-sep' expects a single character (or \\t, \\n).");
        break;
      }
      case 'f':
        r.opts.file = std::string(optarg);
        break;
      case 'c': {
        int n = 0;
        if (!parse_int(optarg, n) || n <= 0) r.errors.push_back("'-c/--column' expects a positive integer.");
        else r.opts.column = n;
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
            r.errors.push_back("'-W/--width' percent must be in 1..100 (e.g. 80%).");
          } else {
            r.opts.width.kind = WidthSpec::Kind::Percent;
            r.opts.width.percent = p;
          }
        } else {
          int w = 0;
          if (!parse_int(s.c_str(), w) || w <= 0) {
            r.errors.push_back("'-W/--width' expects a positive integer (e.g. 80) or percent (e.g. 80%).");
          } else {
            r.opts.width.kind = WidthSpec::Kind::Cols;
            r.opts.width.cols = w;
          }
        }
        break;
      }
      case 'H': {
        int h = 0;
        if (!parse_int(optarg, h) || h <= 0) r.errors.push_back("'-H/--height' expects a positive integer.");
        else r.opts.height = h;
        break;
      }
      case 'T':
        r.opts.title = std::string(optarg);
        break;
      case 'M': {
        double v = 0;
        if (!parse_double(optarg, v)) r.errors.push_back("'-M/--max' expects a number.");
        else r.opts.max_value = v;
        break;
      }
      case 'm': {
        double v = 0;
        if (!parse_double(optarg, v)) r.errors.push_back("'-m/--min' expects a number.");
        else r.opts.min_value = v;
        break;
      }
      case 'S':
        r.opts.force_signed = true;
        break;
      case 'x':
        r.opts.show_x_axis = true;
        if (optarg && *optarg) r.opts.x_axis_fmt = std::string(optarg);
        break;
      case 'y':
        r.opts.show_y_axis = true;
        if (optarg && *optarg) {
          r.opts.y_axis_fmt = std::string(optarg);
          r.opts.y_axis_fmt_explicit = true;
        }
        break;
      case 'Y':
        r.opts.y_axis_fmt = std::string(optarg);
        r.opts.y_axis_fmt_explicit = true;
        break;
      case 1000:
        r.opts.unicode = false;
        break;
      case 1001:
        r.opts.color_mode = Options::ColorMode::Auto;
        break;
      case 1002:
        r.opts.color_mode = Options::ColorMode::Ansi16;
        break;
      case 1003:
        r.opts.color_mode = Options::ColorMode::Ansi256;
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
#else
  r.errors.push_back("Argument parsing not implemented on this platform yet.");
#endif

  return r;
}

} // namespace dotchart
