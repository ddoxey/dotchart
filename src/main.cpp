#include "options.h"
#include "input.h"
#include "render.h"

#include <iostream>

int main(int argc, char** argv) {
  using namespace dotchart;

  auto pr = parse_args(argc, argv);
  if (!pr.errors.empty()) {
    for (const auto& e : pr.errors) std::cerr << "dotchart: " << e << "\n";
    std::cerr << "dotchart: try --help\n";
    return 2;
  }

  if (pr.opts.version) {
    std::cout << version_text() << "\n";
    return 0;
  }
  if (pr.opts.help) {
    std::cout << help_text();
    return 0;
  }

  try {
    const auto text = read_all_input(pr.opts);
    auto parsed = parse_values_with_meta(pr.opts, text);
    auto values = std::move(parsed.values);

    if (pr.opts.show_y_axis && !pr.opts.y_axis_fmt_explicit && parsed.has_first_token) {
      if (parsed.first_has_exp) {
        pr.opts.show_y_axis = false;
      } else if (parsed.first_is_int) {
        pr.opts.y_axis_fmt = "%d";
        pr.opts.y_axis_fmt_is_int = true;
      } else {
        pr.opts.y_axis_fmt = "%." + std::to_string(parsed.first_precision) + "f";
        pr.opts.y_axis_fmt_is_int = false;
      }
    }

    if (values.empty()) {
      std::cerr << "dotchart: no numeric values found in input.\n";
      return 1;
    }

    auto lines = render_chart(pr.opts, values);
    for (const auto& ln : lines) std::cout << ln << "\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "dotchart: " << ex.what() << "\n";
    return 1;
  }
}
