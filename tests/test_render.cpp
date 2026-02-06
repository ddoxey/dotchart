#include "render.h"
#include "braille.h"

#include <gtest/gtest.h>
#include <sstream>

namespace dotchart {
namespace {

static std::string from_u8(const char8_t* s) {
  return std::string(reinterpret_cast<const char*>(s));
}

class RenderChartTest : public ::testing::Test {
protected:
  static const std::vector<double>& curated_sine() {
    static const std::vector<double> data = {
      0.000000, 0.062791, 0.125333, 0.187381, 0.248690,
      0.309017, 0.368125, 0.425779, 0.481754, 0.535827,
      0.587785, 0.637424, 0.684547, 0.728969, 0.770513,
      0.809017, 0.844328, 0.876307, 0.904827, 0.929776,
      0.951057, 0.968583, 0.982287, 1.000000, 1.000000,
      1.000000, 1.000000, 1.000000, 0.982287, 0.968583,
      0.951057, 0.929776, 0.904827, 0.876307, 0.844328,
      0.809017, 0.770513, 0.728969, 0.684547, 0.637424,
      0.587785, 0.535827, 0.481754, 0.425779, 0.368125,
      0.309017, 0.248690, 0.187381, 0.125333, 0.062791,
      0.000000, -0.062791, -0.125333, -0.187381, -0.248690,
      -0.309017, -0.368125, -0.425779, -0.481754, -0.535827,
      -0.587785, -0.637424, -0.684547, -0.728969, -0.770513,
      -0.809017, -0.844328, -0.876307, -0.904827, -0.929776,
      -0.951057, -0.968583, -0.982287, -1.000000, -1.000000,
      -1.000000, -1.000000, -1.000000, -0.982287, -0.968583,
      -0.951057, -0.929776, -0.904827, -0.876307, -0.844328,
      -0.809017, -0.770513, -0.728969, -0.684547, -0.637424,
      -0.587785, -0.535827, -0.481754, -0.425779, -0.368125,
      -0.309017, -0.248690, -0.187381, -0.125333, -0.062791
    };
    return data;
  }
};

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
  EXPECT_EQ(lines[0], from_u8(u8"⡸"));
}

TEST_F(RenderChartTest, CuratedSineOutput) {
  RecordProperty("objective", "Curated sine sample renders expected default chart output.");
  Options opts;
  auto lines = render_chart(opts, curated_sine());
  std::vector<std::string> expected = {
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⣠⣴⣶⣿⣿⣿⣷⣶⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"⠀⠀⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"⠤⠾⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠦⢄⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠃"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠏⠀⠀"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠁⠀⠀⠀"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠁⠀⠀⠀⠀⠀"),
    from_u8(u8"⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠻⠿⣿⣿⣿⡿⠿⠋⠁⠀⠀⠀⠀⠀⠀⠀"),
  };

  ASSERT_EQ(lines.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(lines[i], expected[i]) << "line " << i;
  }
}

TEST_F(RenderChartTest, LabeledCuratedSineOutput) {
  RecordProperty("objective", "Curated sine sample renders expected chart with axis labels.");
  Options opts;
  opts.show_x_axis = true;
  opts.show_y_axis = true;
  opts.y_axis_fmt = "%.6f";
  opts.y_axis_fmt_is_int = false;
  auto lines = render_chart(opts, curated_sine());

    std::vector<std::string> expected = {
    from_u8(u8" 1.000000  ⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⣠⣴⣶⣿⣿⣿⣷⣶⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"             ⠀⠀⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8" 0.555556  ⠤⠤⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"             ⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"          ⠤⠤⠤⠤⠾⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠿⠦⢄⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀"),
    from_u8(u8"             ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠃"),
    from_u8(u8"-0.333333  ⠤⠤⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠏⠀⠀"),
    from_u8(u8"             ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠁⠀⠀⠀"),
    from_u8(u8"             ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠁⠀⠀⠀⠀⠀"),
    from_u8(u8"-1.000000  ⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠻⠿⣿⣿⣿⡿⠿⠋⠁⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"              ⢰ ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰   ⢰"),
    from_u8(u8"              3 7 13 19 25 31 37 43 49 56 62 68 74 80 86 92 100"),
  };


  ASSERT_EQ(lines.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(lines[i], expected[i]) << "line " << i;
  }
}

TEST_F(RenderChartTest, LabeledCuratedSinePointOutput) {
  RecordProperty("objective", "Curated sine sample renders expected point chart with axis labels.");
  Options opts;
  opts.show_x_axis = true;
  opts.show_y_axis = true;
  opts.y_axis_fmt = "%.6f";
  opts.y_axis_fmt_is_int = false;
  opts.style = Options::ChartStyle::Point;
  auto lines = render_chart(opts, curated_sine());

  std::vector<std::string> expected = {
    from_u8(u8" 1.000000  ⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⡠⠔⠒⠉⠉⠉⠑⠒⠤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"             ⠀⠀⠀⠀⠀⢀⠔⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠑⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8" 0.555556  ⠤⠤⠀⠀⠀⢀⠔⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠑⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"             ⠀⢀⠔⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠑⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"          ⠤⠤⠤⠤⠂⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠢⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀"),
    from_u8(u8"             ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠢⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡠⠂"),
    from_u8(u8"-0.333333  ⠤⠤⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⠌⠀⠀"),
    from_u8(u8"             ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠑⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⠔⠁⠀⠀⠀"),
    from_u8(u8"             ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠑⢄⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⠔⠁⠀⠀⠀⠀⠀"),
    from_u8(u8"-1.000000  ⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠢⠤⣀⣀⣀⡠⠤⠊⠁⠀⠀⠀⠀⠀⠀⠀"),
    from_u8(u8"              ⢰ ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰  ⢰   ⢰"),
    from_u8(u8"              3 7 13 19 25 31 37 43 49 56 62 68 74 80 86 92 100"),
  };

  ASSERT_EQ(lines.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(lines[i], expected[i]) << "line " << i;
  }
}

TEST(RenderChart, ZeroLineHeightThree) {
  RecordProperty("objective", "Zero line uses second dot row with height=3.");
  Options opts;
  opts.height = 3;
  opts.show_y_axis = true;
  opts.y_axis_fmt = "%.1f";
  opts.y_axis_fmt_is_int = false;
  std::vector<double> values = {0.0, 0.0};
  auto lines = render_chart(opts, values);

  std::vector<std::string> expected = {
    from_u8(u8"1.0  ⠉⠉⠀"),
    from_u8(u8"0.5  ⠤⠤⠀"),
    from_u8(u8"0.0  ⠤⠤⠤")
  };

  ASSERT_EQ(lines.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(lines[i], expected[i]) << "line " << i;
  }
}

TEST(RenderChart, ZeroLineHeightThreeAsymmetric) {
  RecordProperty("objective", "Zero line alignment with asymmetric range at height=3.");
  Options opts;
  opts.height = 3;
  opts.show_y_axis = true;
  opts.y_axis_fmt = "%.1f";
  opts.y_axis_fmt_is_int = false;
  std::vector<double> values = {0.0, 0.0, 6.0, -5.0};
  auto lines = render_chart(opts, values);

  std::vector<std::string> expected = {
    from_u8(u8" 6.0  ⠉⠉⠀⡇"),
    from_u8(u8"     ⠤⠤⠤⠤⢇"),
    from_u8(u8"-5.0  ⣀⣀⠀⢸")
  };

  ASSERT_EQ(lines.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(lines[i], expected[i]) << "line " << i;
  }
}

} // namespace
} // namespace dotchart
