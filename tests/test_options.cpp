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

}  // namespace
}  // namespace dotchart
