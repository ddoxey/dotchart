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
    auto values = parse_values(pr.opts, text);

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
