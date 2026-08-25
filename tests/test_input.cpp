#include <gtest/gtest.h>

#include "input.h"

namespace dotchart {
namespace {

TEST(InputParseValues, ParsesCommaAndWhitespace) {
  RecordProperty("objective",
                 "Auto delimiter parsing accepts commas and whitespace.");
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
  RecordProperty("objective",
                 "Explicit field separator overrides auto delimiter handling.");
  Options opts;
  opts.field_sep = '|';
  const std::string text = "1|2|3|4\n";
  auto values = parse_values(opts, text);
  ASSERT_EQ(values.size(), 4u);
  EXPECT_DOUBLE_EQ(values[0], 1.0);
  EXPECT_DOUBLE_EQ(values[3], 4.0);
}

TEST(InputParseValues, SkipsInvalidTokens) {
  RecordProperty("objective",
                 "Malformed tokens are ignored without failing parsing.");
  Options opts;
  const std::string text = "1, x, 2, nope, 3";
  auto values = parse_values(opts, text);
  ASSERT_EQ(values.size(), 3u);
  EXPECT_DOUBLE_EQ(values[0], 1.0);
  EXPECT_DOUBLE_EQ(values[1], 2.0);
  EXPECT_DOUBLE_EQ(values[2], 3.0);
}

TEST(InputParseValues, ColumnModeDefaultsToFirstLabelAndSecondValue) {
  RecordProperty("objective",
                 "Selecting either column mode option defaults x=1 and y=2.");
  Options opts;
  opts.column = 2;
  opts.field_sep = ',';
  const auto parsed =
      parse_values_with_meta(opts, "Jan,10.50\nFeb,9.25\nMar,11.00\n");
  ASSERT_EQ(parsed.values.size(), 3u);
  EXPECT_EQ(parsed.x_labels,
            (std::vector<std::string>{"Jan", "Feb", "Mar"}));
  EXPECT_DOUBLE_EQ(parsed.values[0], 10.5);
  EXPECT_DOUBLE_EQ(parsed.values[1], 9.25);
  EXPECT_EQ(parsed.first_precision, 2);
}

TEST(InputParseValues, ColumnModeSupportsColumnOverrides) {
  RecordProperty("objective", "Explicit x and y columns override both defaults.");
  Options opts;
  opts.x_column = 2;
  opts.column = 3;
  opts.field_sep = '|';
  const auto parsed = parse_values_with_meta(
      opts, "ignored|August 1|12.5\nignored|August 2|8.0\n");
  EXPECT_EQ(parsed.x_labels,
            (std::vector<std::string>{"August 1", "August 2"}));
  EXPECT_EQ(parsed.values, (std::vector<double>{12.5, 8.0}));
}

TEST(InputParseValues, ColumnModeSkipsMalformedRecords) {
  RecordProperty("objective",
                 "Records with missing fields, labels, or numeric values are skipped.");
  Options opts;
  opts.x_column = 1;
  opts.field_sep = ',';
  const auto parsed = parse_values_with_meta(
      opts, "Jan,10\nmissing\nFeb,nope\n,12\nMar,8\n");
  EXPECT_EQ(parsed.x_labels, (std::vector<std::string>{"Jan", "Mar"}));
  EXPECT_EQ(parsed.values, (std::vector<double>{10.0, 8.0}));
}

TEST(InputParseValues, ColumnOptionsAreRequiredForRecordMode) {
  RecordProperty("objective",
                 "A separator alone preserves legacy flattened numeric parsing.");
  Options opts;
  opts.field_sep = ',';
  const auto parsed = parse_values_with_meta(opts, "1,10\n2,20\n");
  EXPECT_TRUE(parsed.x_labels.empty());
  EXPECT_EQ(parsed.values, (std::vector<double>{1.0, 10.0, 2.0, 20.0}));
}

}  // namespace
}  // namespace dotchart
