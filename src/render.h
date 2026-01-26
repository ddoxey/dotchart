#pragma once
#include "options.h"
#include <string>
#include <vector>

namespace dotchart {

// Determine width in character columns. If opts.width set, use it.
// Else try tty detection; if not a tty, return fallback 80.
int determine_output_width(const Options& opts);

// Render a v0 braille bar chart. Currently unsigned-only (signed TBD).
std::vector<std::string> render_chart(const Options& opts, const std::vector<double>& values);

} // namespace dotchart
