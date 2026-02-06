#include "render.h"
#include "braille.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#if defined(__unix__) || defined(__APPLE__)
  #include <sys/ioctl.h>
  #include <unistd.h>
#endif

namespace dotchart {

struct TermSize {
  int cols = 80;
  int rows = 24;
  bool is_tty = false;
};

static TermSize determine_terminal_size_fallback80x24() {
  TermSize ts;
#if defined(__unix__) || defined(__APPLE__)
  ts.is_tty = (isatty(fileno(stdout)) != 0);
  if (ts.is_tty) {
    winsize ws{};
    if (ioctl(fileno(stdout), TIOCGWINSZ, &ws) == 0) {
      if (ws.ws_col > 0) ts.cols = static_cast<int>(ws.ws_col);
      if (ws.ws_row > 0) ts.rows = static_cast<int>(ws.ws_row);
    }
  }
#endif
  return ts;
}

struct DataStats {
  double min = 0.0;
  double max = 0.0;
  double max_abs = 0.0;
};

static std::string format_double(const std::string& fmt, double value) {
  char buf[128];
  int n = std::snprintf(buf, sizeof(buf), fmt.c_str(), value);
  if (n < 0) return std::string();
  if (n < static_cast<int>(sizeof(buf))) return std::string(buf, static_cast<size_t>(n));
  std::string out(static_cast<size_t>(n) + 1, '\0');
  std::snprintf(out.data(), out.size(), fmt.c_str(), value);
  out.resize(static_cast<size_t>(n));
  return out;
}

static std::string format_int(const std::string& fmt, int value) {
  char buf[128];
  int n = std::snprintf(buf, sizeof(buf), fmt.c_str(), value);
  if (n < 0) return std::string();
  if (n < static_cast<int>(sizeof(buf))) return std::string(buf, static_cast<size_t>(n));
  std::string out(static_cast<size_t>(n) + 1, '\0');
  std::snprintf(out.data(), out.size(), fmt.c_str(), value);
  out.resize(static_cast<size_t>(n));
  return out;
}

static int clamp_int(int v, int lo, int hi) {
  return std::min(hi, std::max(lo, v));
}

static double safe_range(double lo, double hi) {
  double r = hi - lo;
  return (r > 0.0) ? r : 1.0;
}

static int align_zero_baseline(int baseline, int max_pixels) {
  int b = clamp_int(baseline, 0, max_pixels);
  int aligned = (b / 4) * 4 + 1; // dots 3/6 row within the cell
  if (aligned > max_pixels) aligned -= 4;
  if (aligned < 0) aligned += 4;
  return clamp_int(aligned, 0, max_pixels);
}

static std::string ansi_color_256(int code) {
  return "\x1b[38;5;" + std::to_string(code) + "m";
}

static std::string ansi_color_basic(int code) {
  // Map 0..15 to standard 16-color foreground codes.
  int c = std::clamp(code, 0, 15);
  int ansi = (c < 8) ? (30 + c) : (90 + (c - 8));
  return "\x1b[" + std::to_string(ansi) + "m";
}

static constexpr const char* ANSI_RESET = "\x1b[0m";

static int ramp_color(double t, int c0, int c1) {
  t = std::clamp(1.0 - t, 0.0, 1.0);
  return static_cast<int>(std::lround(c0 + (c1 - c0) * t));
}

enum class RenderColorMode {
  None,
  Ansi16,
  Ansi256
};

static bool env_contains(const char* v, const char* needle) {
  if (!v || !needle) return false;
  return std::string(v).find(needle) != std::string::npos;
}

static int detect_max_colors() {
  const char* colors = std::getenv("TERM_COLORS");
  if (colors && *colors) {
    try {
      return std::stoi(colors);
    } catch (...) {
      return 0;
    }
  }
  const char* term = std::getenv("TERM");
  const char* colorterm = std::getenv("COLORTERM");
  if (term && std::string(term) == "dumb") return 0;
  if (env_contains(term, "256color") || env_contains(colorterm, "256color") ||
      env_contains(colorterm, "truecolor") || env_contains(colorterm, "24bit")) {
    return 256;
  }
  return 16;
}

static RenderColorMode detect_color_mode(Options::ColorMode mode, int max_colors) {
  if (mode == Options::ColorMode::Off) return RenderColorMode::None;
  if (mode == Options::ColorMode::Ansi16) return RenderColorMode::Ansi16;
  if (mode == Options::ColorMode::Ansi256) return RenderColorMode::Ansi256;
  // Auto
  const char* no_color = std::getenv("NO_COLOR");
  if (no_color && *no_color) return RenderColorMode::None;
  const char* clicolor = std::getenv("CLICOLOR");
  if (clicolor && *clicolor == '0') return RenderColorMode::None;
  const char* term = std::getenv("TERM");
  const char* colorterm = std::getenv("COLORTERM");
  if (term && std::string(term) == "dumb") return RenderColorMode::None;
  if (env_contains(term, "256color") || env_contains(colorterm, "256color") ||
      env_contains(colorterm, "truecolor") || env_contains(colorterm, "24bit")) {
    return RenderColorMode::Ansi256;
  }
  if (max_colors > 0 && max_colors < 16) return RenderColorMode::None;
  return RenderColorMode::Ansi16;
}

static std::string color_for_value(double v, double scale_min, double scale_max, RenderColorMode mode) {
  if (mode == RenderColorMode::None) return std::string();
  double denom = (v >= 0.0) ? ((scale_max > 0.0) ? scale_max : 1.0)
                            : ((scale_min < 0.0) ? std::abs(scale_min) : 1.0);
  double t = std::abs(v) / denom;
  t = std::clamp(t, 0.0, 1.0);

  if (mode == RenderColorMode::Ansi256) {
    const int start = 125;
    const int end = 159;
    return ansi_color_256(ramp_color(t, start, end));
  }

  const int start = 1;
  const int end = 14;
  return ansi_color_basic(ramp_color(t, start, end));
}

static bool fmt_expects_int(const std::string& fmt) {
  bool saw_int = false;
  bool saw_float = false;
  for (size_t i = 0; i < fmt.size(); ++i) {
    if (fmt[i] != '%') continue;
    if (i + 1 < fmt.size() && fmt[i + 1] == '%') {
      ++i;
      continue;
    }
    size_t j = i + 1;
    while (j < fmt.size() && std::string("+- #0").find(fmt[j]) != std::string::npos) ++j;
    while (j < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[j]))) ++j;
    if (j < fmt.size() && fmt[j] == '.') {
      ++j;
      while (j < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[j]))) ++j;
    }
    if (j < fmt.size()) {
      char c = fmt[j];
      if (c == 'd' || c == 'i') saw_int = true;
      if (c == 'f' || c == 'F' || c == 'e' || c == 'E' || c == 'g' || c == 'G') saw_float = true;
    }
  }
  return saw_int && !saw_float;
}

static DataStats compute_data_stats(const std::vector<double>& v) {
  DataStats s;
  if (v.empty()) return s;

  double mn = std::numeric_limits<double>::infinity();
  double mx = -std::numeric_limits<double>::infinity();
  double ma = 0.0;

  for (double x : v) {
    mn = std::min(mn, x);
    mx = std::max(mx, x);
    ma = std::max(ma, std::abs(x));
  }
  if (!(ma > 0.0)) ma = 1.0;

  s.min = mn;
  s.max = mx;
  s.max_abs = ma;
  return s;
}

static std::string width_spec_string(const Options& opts) {
  if (opts.width.kind == WidthSpec::Kind::Cols) {
    return std::to_string(opts.width.cols) + " cols";
  }
  if (opts.width.kind == WidthSpec::Kind::Percent) {
    return std::to_string(opts.width.percent) + "%";
  }
  return "auto (fit-to-data)";
}

static void emit_debug_table(const Options& opts,
                             const TermSize& ts,
                             int out_cols_cap,
                             int usable_cols,
                             int cells_w,
                             int height_rows,
                             int target_samples,
                             int input_points,
                             int resampled_points,
                             double scale_max,
                             const DataStats& st,
                             RenderColorMode color_mode,
                             int max_colors) {
  // Print to stderr so dotchart output stays pipeline-friendly.
  auto kv = [&](const char* k, const std::string& v) {
    std::cerr << std::left << std::setw(22) << k << v << "\n";
  };
  auto kvi = [&](const char* k, int v) { kv(k, std::to_string(v)); };
  auto kvd = [&](const char* k, double v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << v;
    kv(k, oss.str());
  };

  std::cerr << "dotchart debug\n";
  std::cerr << "------------------------------\n";
  kv("stdout tty?", ts.is_tty ? "yes" : "no");
  kvi("terminal cols", ts.cols);
  kvi("terminal rows", ts.rows);
  kv("width spec", width_spec_string(opts));
  kvi("out cols cap", out_cols_cap);
  kvi("usable cols", usable_cols);
  kvi("chart cells (W)", cells_w);
  kvi("chart rows (H)", height_rows);
  kvi("target samples", target_samples);
  kvi("input datapoints", input_points);
  kvi("rendered samples", resampled_points);
  kvd("data min", st.min);
  kvd("data max", st.max);
  kvd("data max abs", st.max_abs);
  kvd("scale max used", scale_max);
  const char* cm = (color_mode == RenderColorMode::Ansi256)
                     ? "256"
                     : (color_mode == RenderColorMode::Ansi16 ? "16" : "none");
  kv("color mode", cm);
  if (max_colors > 0) kvi("max colors", max_colors);
  std::cerr << "\n";
}

static int determine_target_columns(const Options& opts, int term_cols) {
  // If user specified -W as columns or percent, use it.
  if (opts.width.kind == WidthSpec::Kind::Cols) {
    return std::max(1, opts.width.cols);
  }
  if (opts.width.kind == WidthSpec::Kind::Percent) {
    const int pct = std::clamp(opts.width.percent, 1, 100);
    int cols = static_cast<int>(std::floor(term_cols * (pct / 100.0)));
    return std::max(1, cols);
  }

  // Auto => "fit-to-data" happens in render_chart() once we know N.
  return term_cols; // just return terminal width cap; actual default width is data-driven.
}

static std::vector<double> resample_maxbin(const std::vector<double>& in, int target) {
  if (target <= 0) return {};
  if (in.empty()) return std::vector<double>(static_cast<size_t>(target), 0.0);
  if (static_cast<int>(in.size()) == target) return in;

  std::vector<double> out(static_cast<size_t>(target), 0.0);
  const int n = static_cast<int>(in.size());

  for (int j = 0; j < target; ++j) {
    int start = static_cast<int>(std::floor((static_cast<double>(j)     * n) / target));
    int end   = static_cast<int>(std::floor((static_cast<double>(j + 1) * n) / target));
    start = std::clamp(start, 0, n);
    end   = std::clamp(end,   0, n);
    if (end <= start) end = std::min(n, start + 1);

    double m = in[static_cast<size_t>(start)];
    for (int k = start + 1; k < end; ++k) {
      m = std::max(m, in[static_cast<size_t>(k)]);
    }
    out[static_cast<size_t>(j)] = m;
  }

  return out;
}

static std::vector<double> resample_extreme_abs(const std::vector<double>& in, int target) {
  if (target <= 0) return {};
  if (in.empty()) return std::vector<double>(static_cast<size_t>(target), 0.0);
  if (static_cast<int>(in.size()) == target) return in;

  std::vector<double> out(static_cast<size_t>(target), 0.0);
  const int n = static_cast<int>(in.size());

  for (int j = 0; j < target; ++j) {
    int start = static_cast<int>(std::floor((static_cast<double>(j)     * n) / target));
    int end   = static_cast<int>(std::floor((static_cast<double>(j + 1) * n) / target));
    start = std::clamp(start, 0, n);
    end   = std::clamp(end,   0, n);
    if (end <= start) end = std::min(n, start + 1);

    double pick = in[static_cast<size_t>(start)];
    double pick_abs = std::abs(pick);
    for (int k = start + 1; k < end; ++k) {
      double v = in[static_cast<size_t>(k)];
      double a = std::abs(v);
      if (a > pick_abs) {
        pick = v;
        pick_abs = a;
      }
    }
    out[static_cast<size_t>(j)] = pick;
  }

  return out;
}

static void set_bar_segment(BrailleCanvas& canvas,
                            int cx,
                            int px,
                            int start_from_bottom,
                            int end_from_bottom) {
  const int H = canvas.height();
  const int max_pixels = H * 4;
  int a = clamp_int(start_from_bottom, 0, max_pixels);
  int b = clamp_int(end_from_bottom, 0, max_pixels);
  if (b <= a) return;

  for (int p = a; p < b; ++p) {
    int cy_from_bottom = p / 4;
    int local_py_from_bottom = p % 4;
    int cy = (H - 1) - cy_from_bottom;
    int py = 3 - local_py_from_bottom;
    canvas.set_pixel(cx, cy, px, py);
  }
}

std::vector<std::string> render_chart(const Options& opts, const std::vector<double>& values) {
  // v0: unsigned-only bars (negatives clamped to 0), but width behavior improved.
  const TermSize ts = determine_terminal_size_fallback80x24();
  const int out_cols_cap = determine_target_columns(opts, ts.cols);

  const int H = std::max(1, opts.height);

  const DataStats st = compute_data_stats(values);

  double scale_min = opts.min_value.value_or(st.min);
  double scale_max = opts.max_value.value_or(st.max);
  if (scale_min > scale_max) std::swap(scale_min, scale_max);

  const bool signed_mode = opts.force_signed || (scale_min < 0.0);
  if (opts.force_signed) {
    if (scale_min > 0.0) scale_min = 0.0;
    if (scale_max < 0.0) scale_max = 0.0;
  }

  double M = 1.0;
  if (!signed_mode) {
    double auto_max = std::max(0.0, st.max);
    M = opts.max_value.value_or(auto_max);
    if (M == 0.0) M = 1.0;
    scale_min = 0.0;
    scale_max = M;
  }

  int y_axis_cols = 0;
  std::vector<std::string> y_prefix;
  if (opts.show_y_axis) {
    const int P = H * 4;
    const double range = safe_range(scale_min, scale_max);
    int baseline = static_cast<int>(std::lround(((0.0 - scale_min) / range) * P));
    baseline = align_zero_baseline(baseline, P);
    const int zero_row = clamp_int(H - 1 - (baseline / 4), 0, H - 1);

    std::vector<std::string> labels(static_cast<size_t>(H));
    std::vector<bool> has_label(static_cast<size_t>(H), false);
    size_t max_label = 0;

    std::vector<bool> want_label(static_cast<size_t>(H), false);
    if (H <= 3) {
      for (int row = 0; row < H; ++row) want_label[static_cast<size_t>(row)] = true;
    } else {
      want_label[0] = true;
      want_label[static_cast<size_t>(H - 1)] = true;
      for (int row = 1; row < H - 1; ++row) {
        if (want_label[static_cast<size_t>(row - 1)]) continue;
        if (want_label[static_cast<size_t>(row + 1)]) continue;
        want_label[static_cast<size_t>(row)] = true;
      }
    }

    const double step = (H > 1) ? (range / (H - 1)) : 0.0;
    for (int row = 0; row < H; ++row) {
      if (!want_label[static_cast<size_t>(row)]) continue;
      if (signed_mode && row == zero_row) continue;

      double v = scale_max - step * row;
      std::string label;
      if (opts.y_axis_fmt_is_int || fmt_expects_int(opts.y_axis_fmt)) {
        label = format_int(opts.y_axis_fmt, static_cast<int>(std::lround(v)));
      } else {
        label = format_double(opts.y_axis_fmt, v);
      }
      if (label.empty()) continue;
      max_label = std::max(max_label, label.size());
      labels[static_cast<size_t>(row)] = std::move(label);
      has_label[static_cast<size_t>(row)] = true;
    }

    const int tick_len = 3; // allows "⠤⠤⠤" for zero row when unlabeled
    y_axis_cols = static_cast<int>(max_label) + 1 + tick_len; // label + space + tick
    y_prefix.resize(static_cast<size_t>(H));
    const std::string zero_tick = utf8_encode(0x2824);   // dots 3 and 6 (row 3)
    const std::string top_tick = utf8_encode(0x2809);    // dots 1 and 4 (row 1)
    const std::string bottom_tick = utf8_encode(0x28C0); // dots 7 and 8 (row 4)
    for (int row = 0; row < H; ++row) {
      std::string label = labels[static_cast<size_t>(row)];
      if (label.size() < max_label) {
        label.insert(label.begin(), max_label - label.size(), ' ');
      }
      std::string tick = "   ";
      if (row == zero_row) {
        if (has_label[static_cast<size_t>(row)]) {
          tick = " " + zero_tick + zero_tick;
        } else if (signed_mode) {
          tick = zero_tick + zero_tick + zero_tick;
        }
      } else if (has_label[static_cast<size_t>(row)]) {
        const std::string& mark =
          (row == 0) ? top_tick : (row == H - 1 ? bottom_tick : zero_tick);
        tick = " " + mark + mark;
      }
      y_prefix[static_cast<size_t>(row)] = label + " " + tick;
    }
  }

  const int usable_cols = std::max(1, out_cols_cap - y_axis_cols);

  // Each braille cell column represents 2 samples.
  const int n = static_cast<int>(values.size());

  // Default width: fit-to-data => ceil(n/2), capped by usable_cols.
  int cells_w = 1;
  if (opts.width.kind == WidthSpec::Kind::Auto) {
    int need = std::max(1, (n + 1) / 2);
    cells_w = std::min(usable_cols, need);
  } else {
    // Explicit width (cols or percent): fill that width (capped).
    cells_w = usable_cols;
  }

  // Target sample count for rendering: 2 per cell.
  const int target_samples = cells_w * 2;

  // If explicit width was provided, scale data to fit exactly.
  // If auto width, we only need to scale if data exceeds what we can show.
  std::vector<double> samples;
  if (opts.width.kind == WidthSpec::Kind::Auto) {
    if (n <= target_samples) {
      samples = values; // no resample needed; chart width will match ceil(n/2)
    } else {
      samples = resample_maxbin(values, target_samples);
    }
  } else {
    samples = resample_maxbin(values, target_samples);
  }

  if (signed_mode && (opts.width.kind != WidthSpec::Kind::Auto || n > target_samples)) {
    samples = resample_extreme_abs(values, target_samples);
  }

  const int max_colors = detect_max_colors();
  const RenderColorMode color_mode = detect_color_mode(opts.color_mode, max_colors);

  if (opts.debug) {
    emit_debug_table(opts,
                     ts,
                     out_cols_cap,
                     usable_cols,
                     cells_w,
                     H,
                     target_samples,
                     static_cast<int>(values.size()),
                     static_cast<int>(samples.size()),
                     signed_mode ? scale_max : M,
                     st,
                     color_mode,
                     max_colors);
    if (signed_mode) {
      const int P = H * 4;
      double range = scale_max - scale_min;
      if (!(range > 0.0)) range = 1.0;
      int baseline = static_cast<int>(std::lround(((0.0 - scale_min) / range) * P));
      baseline = align_zero_baseline(baseline, P);
      std::cerr << std::left << std::setw(22) << "signed mode" << "yes" << "\n";
      std::cerr << std::left << std::setw(22) << "scale min" << scale_min << "\n";
      std::cerr << std::left << std::setw(22) << "scale max" << scale_max << "\n";
      std::cerr << std::left << std::setw(22) << "baseline px" << baseline << "\n\n";
    } else {
      std::cerr << std::left << std::setw(22) << "signed mode" << "no" << "\n\n";
    }
  }

  const int P = H * 4; // pixel rows

  BrailleCanvas canvas(cells_w, H);
  const bool point_mode = (opts.style == Options::ChartStyle::Point);
  auto set_point = [&](int cx, int px, int p) {
    set_bar_segment(canvas, cx, px, p, p + 1);
  };

  // We always render up to target_samples, but if auto+no-resample and samples shorter,
  // render only those samples (the width will already be tight).
  const int take = std::min(static_cast<int>(samples.size()), target_samples);

  double range = safe_range(scale_min, scale_max);
  int baseline = static_cast<int>(std::lround(((0.0 - scale_min) / range) * P));
  baseline = align_zero_baseline(baseline, P);

  if (!signed_mode) {
    for (int i = 0; i < take; ++i) {
      int cx = i / 2;
      int px = i % 2;
      double v = samples[static_cast<size_t>(i)];
      double clamped = std::clamp(v, 0.0, M);
      if (point_mode) {
        if (std::abs(clamped) < 1e-12) {
          set_point(cx, px, baseline);
          continue;
        }
        double y = (M > 0.0) ? (clamped / M) : 0.0;
        int pos = baseline + static_cast<int>(std::lround(y * (P - baseline)));
        pos = clamp_int(pos, 0, P);
        int p = std::max(baseline, pos - 1);
        set_point(cx, px, p);
        continue;
      }
      if (std::abs(clamped) < 1e-12) {
        set_bar_segment(canvas, cx, px, baseline, baseline + 1);
        continue;
      }
      double y = (M > 0.0) ? (clamped / M) : 0.0;
      int pos = baseline + static_cast<int>(std::lround(y * (P - baseline)));
      pos = clamp_int(pos, 0, P);
      set_bar_segment(canvas, cx, px, baseline, pos);
    }
  } else {
    for (int i = 0; i < take; ++i) {
      int cx = i / 2;
      int px = i % 2;
      double v = samples[static_cast<size_t>(i)];
      double clamped = std::clamp(v, scale_min, scale_max);
      if (point_mode) {
        if (std::abs(clamped) < 1e-12) {
          set_point(cx, px, baseline);
          continue;
        }
        int pos = baseline;
        if (clamped >= 0.0) {
          double denom = (scale_max > 0.0) ? scale_max : 1.0;
          double y = clamped / denom;
          pos = baseline + static_cast<int>(std::lround(y * (P - baseline)));
          pos = clamp_int(pos, 0, P);
          int p = std::max(baseline, pos - 1);
          set_point(cx, px, p);
        } else {
          double denom = (scale_min < 0.0) ? std::abs(scale_min) : 1.0;
          double y = std::abs(clamped) / denom;
          pos = baseline - static_cast<int>(std::lround(y * baseline));
          pos = clamp_int(pos, 0, P);
          set_point(cx, px, pos);
        }
        continue;
      }
      if (std::abs(clamped) < 1e-12) {
        set_bar_segment(canvas, cx, px, baseline, baseline + 1);
        continue;
      }
      int pos = baseline;
      if (clamped >= 0.0) {
        double denom = (scale_max > 0.0) ? scale_max : 1.0;
        double y = clamped / denom;
        pos = baseline + static_cast<int>(std::lround(y * (P - baseline)));
      } else {
        double denom = (scale_min < 0.0) ? std::abs(scale_min) : 1.0;
        double y = std::abs(clamped) / denom;
        pos = baseline - static_cast<int>(std::lround(y * baseline));
      }
      pos = clamp_int(pos, 0, P);

      if (clamped >= 0.0) {
        int end = std::max(baseline, pos);
        set_bar_segment(canvas, cx, px, baseline, end);
      } else {
        int start = std::min(baseline, pos);
        set_bar_segment(canvas, cx, px, start, baseline);
      }
    }
  }

  auto lines = canvas.render_utf8();

  std::vector<std::string> out;
  out.reserve(lines.size() + (opts.title ? 1 : 0) + (opts.show_x_axis ? 2 : 0));

  if (opts.title) out.push_back(*opts.title);

  if (opts.show_y_axis && !y_prefix.empty()) {
    for (size_t i = 0; i < lines.size(); ++i) {
      if (color_mode != RenderColorMode::None) {
        int row = static_cast<int>(i);
      double v = scale_max - (safe_range(scale_min, scale_max) / H) * (row + 1);
        std::string color = color_for_value(v, scale_min, scale_max, color_mode);
        out.push_back(y_prefix[i] + color + lines[i] + ANSI_RESET);
      } else {
        out.push_back(y_prefix[i] + lines[i]);
      }
    }
  } else {
    if (color_mode != RenderColorMode::None) {
      for (size_t i = 0; i < lines.size(); ++i) {
        int row = static_cast<int>(i);
        double v = scale_max - (safe_range(scale_min, scale_max) / H) * (row + 1);
        std::string color = color_for_value(v, scale_min, scale_max, color_mode);
        out.push_back(color + lines[i] + ANSI_RESET);
      }
    } else {
      out.insert(out.end(), lines.begin(), lines.end());
    }
  }

  if (opts.show_x_axis) {
    const int width = cells_w;
    const std::string tick_glyph = utf8_encode(0x28B0); // dots 5, 6, 8
    std::vector<bool> tick_cols(static_cast<size_t>(width), false);
    std::string label_line(static_cast<size_t>(width), ' ');

    const int denom = std::max(1, n - 1);
    struct Placement {
      int col = 0;
      int start = 0;
      int end = 0;
      std::string label;
    };
    std::vector<Placement> placements;

    int col = width - 1;
    std::string last_label;
    while (col >= 0) {
      double t = (width == 1) ? 0.0 : static_cast<double>(col) / (width - 1);
      int idx = static_cast<int>(std::lround(t * denom));
      int label_value = idx + 1;
      std::string label = format_int(opts.x_axis_fmt, label_value);
      if (label.empty()) {
        --col;
        continue;
      }
      if (label == last_label) {
        --col;
        continue;
      }
      int label_len = static_cast<int>(label.size());
      int end = col;
      int start = end - (label_len - 1);
      if (start < 0) break;
      placements.push_back(Placement{col, start, end, label});
      last_label = label;
      col = start - 2; // one space gap between labels
    }

    for (auto it = placements.rbegin(); it != placements.rend(); ++it) {
      const auto& p = *it;
      for (size_t k = 0; k < p.label.size(); ++k) {
        label_line[static_cast<size_t>(p.start) + k] = p.label[k];
      }
      tick_cols[static_cast<size_t>(p.col)] = true;
    }

    std::string left_pad(static_cast<size_t>(y_axis_cols), ' ');
    std::string tick_line;
    tick_line.reserve(static_cast<size_t>(width) * tick_glyph.size());
    for (int i = 0; i < width; ++i) {
      tick_line += tick_cols[static_cast<size_t>(i)] ? tick_glyph : " ";
    }
    out.push_back(left_pad + tick_line);
    out.push_back(left_pad + label_line);
  }

  return out;
}

} // namespace dotchart
