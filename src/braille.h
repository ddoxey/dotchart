#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace dotchart {

// Braille is a 2x4 dot cell. We store per-cell dot masks (8 bits).
// Code point = 0x2800 + mask.
class BrailleCanvas {
 public:
  BrailleCanvas(int width_cells, int height_cells);

  int width() const { return m_w; }
  int height() const { return m_h; }

  // Set a dot pixel at cell (cx, cy), local pixel (px in {0,1}, py in {0..3}),
  // where py=0 is top dot row.
  void set_pixel(int cx, int cy, int px, int py);

  // Set a vertical bar for a single sample at cell column cx, using half-column
  // px (0=left, 1=right), with height in pixels (0..height*4) bottom-up.
  void set_bar_half(int cx, int px, int pixel_height);

  // Render rows of UTF-8 strings, top-to-bottom.
  std::vector<std::string> render_utf8() const;

 private:
  int m_w = 0;
  int m_h = 0;
  std::vector<uint8_t> m_mask;  // m_h * m_w

  uint8_t& cell(int x, int y) { return m_mask[y * m_w + x]; }
  uint8_t cell(int x, int y) const { return m_mask[y * m_w + x]; }
};

// Encode a single Unicode codepoint (<= 0x10FFFF) into UTF-8.
std::string utf8_encode(char32_t cp);

}  // namespace dotchart
