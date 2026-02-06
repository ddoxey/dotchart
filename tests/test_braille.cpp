#include <gtest/gtest.h>

#include "braille.h"

namespace dotchart {
namespace {

TEST(BrailleCanvas, SingleDotMatchesCodepoint) {
  RecordProperty("objective",
                 "Single pixel maps to expected Braille codepoint.");
  BrailleCanvas canvas(1, 1);
  canvas.set_pixel(0, 0, 0, 0);  // left column, top dot => dot 1
  auto lines = canvas.render_utf8();
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines[0], utf8_encode(0x2801));
}

TEST(BrailleCanvas, MultipleDotsCombineInSingleCell) {
  RecordProperty("objective",
                 "Multiple pixels combine into one Braille cell mask.");
  BrailleCanvas canvas(1, 1);
  canvas.set_pixel(0, 0, 0, 3);  // dot 7
  canvas.set_pixel(0, 0, 1, 0);  // dot 4
  auto lines = canvas.render_utf8();
  ASSERT_EQ(lines.size(), 1u);
  char32_t cp = 0x2800u + (1u << (7 - 1)) + (1u << (4 - 1));
  EXPECT_EQ(lines[0], utf8_encode(cp));
}

}  // namespace
}  // namespace dotchart
