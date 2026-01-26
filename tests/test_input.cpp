#include "input.h"

#include <gtest/gtest.h>

namespace dotchart {
namespace {

TEST(InputParseValues, ParsesCommaAndWhitespace) {
  RecordProperty("objective", "Auto delimiter parsing accepts commas and whitespace.");
  Options opts;
  opts.field_sep = '\0';
  const std::string text = "1, 2  3\n4\t5";
  auto values = parse_values(opts, text);
  ASSERT_EQ(values.size(), 5u);
  EXPECT_DOUBLE_EQ(values[0], 1.0);
  EXPECT_DOUBLE_EQ(values[1], 2.0);
  EXPECT_DOUBLE_EQ(values[2], 3.0);
  EXPECT_DOUBLE_EQ(values[3], 4.0);
  EXPECT_DOUBLE_EQ(values[4], 5.0);
}

TEST(InputParseValues, ParsesCustomSeparator) {
  RecordProperty("objective", "Explicit field separator overrides auto delimiter handling.");
  Options opts;
  opts.field_sep = '|';
  const std::string text = "1|2|3|4";
  auto values = parse_values(opts, text);
  ASSERT_EQ(values.size(), 4u);
  EXPECT_DOUBLE_EQ(values[0], 1.0);
  EXPECT_DOUBLE_EQ(values[3], 4.0);
}

TEST(InputParseValues, SkipsInvalidTokens) {
  RecordProperty("objective", "Malformed tokens are ignored without failing parsing.");
  Options opts;
  const std::string text = "1, x, 2, nope, 3";
  auto values = parse_values(opts, text);
  ASSERT_EQ(values.size(), 3u);
  EXPECT_DOUBLE_EQ(values[0], 1.0);
  EXPECT_DOUBLE_EQ(values[1], 2.0);
  EXPECT_DOUBLE_EQ(values[2], 3.0);
}

} // namespace
} // namespace dotchart
