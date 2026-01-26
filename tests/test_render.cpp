#include "render.h"
#include "braille.h"

#include <gtest/gtest.h>
#include <sstream>

namespace dotchart {
namespace {

static std::vector<std::string> split_lines(const std::string& text) {
  std::vector<std::string> out;
  std::stringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    out.push_back(line);
  }
  return out;
}

static std::string from_u8(const char8_t* s) {
  return std::string(reinterpret_cast<const char*>(s));
}

TEST(RenderChart, UnsignedAutoWidthHeight) {
  RecordProperty("objective", "Unsigned render keeps expected row count and cell width.");
  Options opts;
  opts.height = 2;
  std::vector<double> values = {0.0, 1.0};
  auto lines = render_chart(opts, values);
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0].size(), 3u);
  EXPECT_EQ(lines[1].size(), 3u);
}

TEST(RenderChart, SignedBaselineBoundary) {
  RecordProperty("objective", "Signed render places baseline as a pixel boundary.");
  Options opts;
  opts.height = 1;
  opts.force_signed = true;
  std::vector<double> values = {-1.0, 1.0};
  auto lines = render_chart(opts, values);
  ASSERT_EQ(lines.size(), 1u);
  char32_t cp = 0x2800u;
  cp += (1u << (7 - 1)); // left bottom dot
  cp += (1u << (3 - 1)); // left mid dot
  cp += (1u << (5 - 1)); // right mid dot
  cp += (1u << (4 - 1)); // right top dot
  EXPECT_EQ(lines[0], utf8_encode(cp));
}

TEST(RenderChart, CuratedSineOutput) {
  RecordProperty("objective", "Curated sine sample renders expected default chart output.");
  Options opts;
  std::vector<double> values = {
    0.000000, 0.062791, 0.125333, 0.187381, 0.248690,
    0.309017, 0.368125, 0.425779, 0.481754, 0.535827,
    0.587785, 0.637424, 0.684547, 0.728969, 0.770513,
    0.809017, 0.844328, 0.876307, 0.904827, 0.929776,
    0.951057, 0.968583, 0.982287, 0.992115, 0.998027,
    1.000000, 0.998027, 0.992115, 0.982287, 0.968583,
    0.951057, 0.929776, 0.904827, 0.876307, 0.844328,
    0.809017, 0.770513, 0.728969, 0.684547, 0.637424,
    0.587785, 0.535827, 0.481754, 0.425779, 0.368125,
    0.309017, 0.248690, 0.187381, 0.125333, 0.062791,
    0.000000, -0.062791, -0.125333, -0.187381, -0.248690,
    -0.309017, -0.368125, -0.425779, -0.481754, -0.535827,
    -0.587785, -0.637424, -0.684547, -0.728969, -0.770513,
    -0.809017, -0.844328, -0.876307, -0.904827, -0.929776,
    -0.951057, -0.968583, -0.982287, -0.992115, -0.998027,
    -1.000000, -0.998027, -0.992115, -0.982287, -0.968583,
    -0.951057, -0.929776, -0.904827, -0.876307, -0.844328,
    -0.809017, -0.770513, -0.728969, -0.684547, -0.637424,
    -0.587785, -0.535827, -0.481754, -0.425779, -0.368125,
    -0.309017, -0.248690, -0.187381, -0.125333, -0.062791
  };

  auto lines = render_chart(opts, values);
  std::vector<std::string> expected = {
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⣠⣴⣶⣿⣿⣿⣷⣶⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"⠀⠀⠀⠀⠀⢀⣴⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"⠀⠀⣠⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"⢀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠏"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠁⠀"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋⠀⠀⠀"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠻⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠋⠀⠀⠀⠀⠀"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠻⠿⣿⣿⣿⡿⠿⠛⠁⠀⠀⠀⠀⠀⠀⠀")
  };

  ASSERT_EQ(lines.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(lines[i], expected[i]) << "line " << i;
  }
}

} // namespace
} // namespace dotchart
