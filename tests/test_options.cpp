#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "options.h"

namespace dotchart {
namespace {

static std::vector<char*> make_argv(std::vector<std::string>& storage) {
  std::vector<char*> argv;
  argv.reserve(storage.size() + 1);
  for (auto& s : storage) argv.push_back(s.data());
  argv.push_back(nullptr);
  return argv;
}

TEST(ParseArgs, YAxisNoFormatKeepsDefault) {
  RecordProperty("objective",
                 "Optional -y without format keeps default and no file.");
  std::vector<std::string> storage = {"dotchart", "-y"};
  auto argv = make_argv(storage);
  ParseResult r = parse_args(static_cast<int>(storage.size()), argv.data());
  ASSERT_TRUE(r.errors.empty());
  EXPECT_TRUE(r.opts.show_y_axis);
  EXPECT_FALSE(r.opts.y_axis_fmt_explicit);
  EXPECT_EQ(r.opts.y_axis_fmt, "%3.0f ");
  EXPECT_FALSE(r.opts.file.has_value());
}

TEST(ParseArgs, YAxisFormatAsNextArg) {
  RecordProperty("objective",
                 "Optional -y consumes a following format string.");
  std::vector<std::string> storage = {"dotchart", "-y", "%0.1f"};
  auto argv = make_argv(storage);
  ParseResult r = parse_args(static_cast<int>(storage.size()), argv.data());
  ASSERT_TRUE(r.errors.empty());
  EXPECT_TRUE(r.opts.show_y_axis);
  EXPECT_TRUE(r.opts.y_axis_fmt_explicit);
  EXPECT_EQ(r.opts.y_axis_fmt, "%0.1f");
}

TEST(ParseArgs, YAxisDoesNotConsumeFilename) {
  RecordProperty("objective",
                 "Optional -y does not consume non-format file arg.");
  std::vector<std::string> storage = {"dotchart", "-y", "data.csv"};
  auto argv = make_argv(storage);
  ParseResult r = parse_args(static_cast<int>(storage.size()), argv.data());
  ASSERT_TRUE(r.errors.empty());
  EXPECT_TRUE(r.opts.show_y_axis);
  EXPECT_FALSE(r.opts.y_axis_fmt_explicit);
  EXPECT_EQ(r.opts.y_axis_fmt, "%3.0f ");
  ASSERT_TRUE(r.opts.file.has_value());
  EXPECT_EQ(*r.opts.file, "data.csv");
}

TEST(ParseArgs, XAxisFormatAsNextArg) {
  RecordProperty("objective",
                 "Optional -x consumes a following format string.");
  std::vector<std::string> storage = {"dotchart", "-x", "%03d"};
  auto argv = make_argv(storage);
  ParseResult r = parse_args(static_cast<int>(storage.size()), argv.data());
  ASSERT_TRUE(r.errors.empty());
  EXPECT_TRUE(r.opts.show_x_axis);
  EXPECT_EQ(r.opts.x_axis_fmt, "%03d");
}

TEST(ParseArgs, ColorRangeSpec) {
  RecordProperty("objective",
                 "--color range sets 256-color ramp and enables color.");
  std::vector<std::string> storage = {"dotchart", "--color", "160..195"};
  auto argv = make_argv(storage);
  ParseResult r = parse_args(static_cast<int>(storage.size()), argv.data());
  ASSERT_TRUE(r.errors.empty());
  EXPECT_EQ(r.opts.color_mode, Options::ColorMode::Ansi256);
  ASSERT_EQ(r.opts.color_ramp.size(), 36u);
  EXPECT_EQ(r.opts.color_ramp.front(), 160);
  EXPECT_EQ(r.opts.color_ramp.back(), 195);
}

TEST(ParseArgs, ColorListSpec) {
  RecordProperty("objective", "--color list sets explicit 256-color ramp.");
  std::vector<std::string> storage = {"dotchart", "--color", "160,167,174,181"};
  auto argv = make_argv(storage);
  ParseResult r = parse_args(static_cast<int>(storage.size()), argv.data());
  ASSERT_TRUE(r.errors.empty());
  EXPECT_EQ(r.opts.color_mode, Options::ColorMode::Ansi256);
  std::vector<int> expected = {160, 167, 174, 181};
  EXPECT_EQ(r.opts.color_ramp, expected);
}

TEST(ParseArgs, ColorDoesNotConsumeFilename) {
  RecordProperty("objective",
                 "--color without spec does not consume file argument.");
  std::vector<std::string> storage = {"dotchart", "--color", "data.csv"};
  auto argv = make_argv(storage);
  ParseResult r = parse_args(static_cast<int>(storage.size()), argv.data());
  ASSERT_TRUE(r.errors.empty());
  EXPECT_EQ(r.opts.color_mode, Options::ColorMode::Auto);
  EXPECT_TRUE(r.opts.color_ramp.empty());
  ASSERT_TRUE(r.opts.file.has_value());
  EXPECT_EQ(*r.opts.file, "data.csv");
}

}  // namespace
}  // namespace dotchart
