#include "braille.h"

#include <algorithm>

namespace dotchart {

// dot mapping: (px,py) -> dot number
// left column: 1,2,3,7 ; right: 4,5,6,8
static constexpr int DOTS[2][4] = {{1, 2, 3, 7}, {4, 5, 6, 8}};

std::string utf8_encode(char32_t cp) {
  std::string out;
  if (cp <= 0x7F) {
    out.push_back(static_cast<char>(cp));
  } else if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
  return out;
}

BrailleCanvas::BrailleCanvas(int width_cells, int height_cells)
    : m_w(std::max(0, width_cells)),
      m_h(std::max(0, height_cells)),
      m_mask(static_cast<size_t>(m_w * m_h), 0) {}

void BrailleCanvas::set_pixel(int cx, int cy, int px, int py) {
  if (cx < 0 || cx >= m_w || cy < 0 || cy >= m_h) return;
  if (px < 0 || px > 1 || py < 0 || py > 3) return;
  int dot = DOTS[px][py];  // 1..8
  cell(cx, cy) |= static_cast<uint8_t>(1u << (dot - 1));
}

void BrailleCanvas::set_bar_half(int cx, int px, int pixel_height) {
  if (cx < 0 || cx >= m_w) return;
  if (px < 0 || px > 1) return;

  int max_pixels = m_h * 4;
  int h = std::clamp(pixel_height, 0, max_pixels);

  // Fill bottom-up: for each pixel row from 0..h-1, set corresponding dot.
  // Canvas cell row cy: 0 is top; pixel row 0 is bottom -> map carefully.
  for (int p = 0; p < h; ++p) {
    int global_py_from_bottom = p;  // 0..max_pixels-1 (bottom-up)
    int cy_from_bottom = global_py_from_bottom / 4;        // 0..m_h-1
    int local_py_from_bottom = global_py_from_bottom % 4;  // 0..3
    int cy = (m_h - 1) - cy_from_bottom;  // convert to top-down index
    int py = 3 - local_py_from_bottom;    // local py where 0 is top
    set_pixel(cx, cy, px, py);
  }
}

std::vector<std::string> BrailleCanvas::render_utf8() const {
  std::vector<std::string> lines;
  lines.reserve(static_cast<size_t>(m_h));
  for (int y = 0; y < m_h; ++y) {
    std::string row;
    row.reserve(static_cast<size_t>(m_w * 3));  // braille UTF-8 is 3 bytes
    for (int x = 0; x < m_w; ++x) {
      char32_t cp = 0x2800u + static_cast<char32_t>(cell(x, y));
      row += utf8_encode(cp);
    }
    lines.push_back(std::move(row));
  }
  return lines;
}

}  // namespace dotchart
