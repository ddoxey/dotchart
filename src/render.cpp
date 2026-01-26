#include "render.h"
#include "braille.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
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
                             const DataStats& st) {
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

static int clamp_int(int v, int lo, int hi) {
  return std::min(hi, std::max(lo, v));
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

  // Y-axis not implemented yet; keep at 0 for now.
  const int y_axis_cols = 0;
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

  const DataStats st = compute_data_stats(values);

  double scale_min = opts.min_value.value_or(st.min);
  double scale_max = opts.max_value.value_or(st.max);
  if (scale_min > scale_max) std::swap(scale_min, scale_max);

  const bool signed_mode = opts.force_signed || (scale_min < 0.0);
  if (opts.force_signed) {
    if (scale_min > 0.0) scale_min = 0.0;
    if (scale_max < 0.0) scale_max = 0.0;
  }

  if (signed_mode && (opts.width.kind != WidthSpec::Kind::Auto || n > target_samples)) {
    samples = resample_extreme_abs(values, target_samples);
  }

  double M = 1.0;
  if (!signed_mode) {
    double auto_max = std::max(0.0, st.max);
    M = opts.max_value.value_or(auto_max);
    if (M == 0.0) M = 1.0;
  }

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
                     st);
    if (signed_mode) {
      const int P = H * 4;
      double range = scale_max - scale_min;
      if (!(range > 0.0)) range = 1.0;
      int baseline = static_cast<int>(std::lround(((0.0 - scale_min) / range) * P));
      baseline = clamp_int(baseline, 0, P);
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

  // We always render up to target_samples, but if auto+no-resample and samples shorter,
  // render only those samples (the width will already be tight).
  const int take = std::min(static_cast<int>(samples.size()), target_samples);

  if (!signed_mode) {
    for (int i = 0; i < take; ++i) {
      int cx = i / 2;
      int px = i % 2;
      double v = samples[static_cast<size_t>(i)];
      double clamped = std::clamp(v, 0.0, M);
      int pix = static_cast<int>(std::lround((clamped / M) * P));
      canvas.set_bar_half(cx, px, pix);
    }
  } else {
    double range = scale_max - scale_min;
    if (!(range > 0.0)) range = 1.0;
    int baseline = static_cast<int>(std::lround(((0.0 - scale_min) / range) * P));
    baseline = clamp_int(baseline, 0, P);

    for (int i = 0; i < take; ++i) {
      int cx = i / 2;
      int px = i % 2;
      double v = samples[static_cast<size_t>(i)];
      double clamped = std::clamp(v, scale_min, scale_max);
      double y = ((clamped - scale_min) / range) * P; // pixels from bottom
      int pos = static_cast<int>(std::lround(y));
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

  if (opts.title) {
    std::vector<std::string> out;
    out.reserve(lines.size() + 1);
    out.push_back(*opts.title);
    out.insert(out.end(), lines.begin(), lines.end());
    return out;
  }
  return lines;
}

} // namespace dotchart
