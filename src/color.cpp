//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include <ftxui/screen/color.hpp>

#include <cstdint>
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <string>

namespace roguey
{
  ftxui::Color parse_hex_color(std::string const& hex_str)
  {
    if (hex_str.empty() || hex_str[0] != '#') return ftxui::Color::White;

    std::string clean = hex_str.substr(1);
    int r = 0, g = 0, b = 0;

    if (clean.length() == 6)
    {
      std::stringstream ss;
      ss << std::hex << clean;
      unsigned int value;
      ss >> value;
      r = (value >> 16) & 0xFF;
      g = (value >> 8) & 0xFF;
      b = value & 0xFF;
    }
    else if (clean.length() == 3)
    {
      std::stringstream ss;
      ss << std::hex << clean;
      unsigned int value;
      ss >> value;
      // Expand 0xRGB to 0xRRGGBB
      r = ((value >> 8) & 0xF) * 17;
      g = ((value >> 4) & 0xF) * 17;
      b = (value & 0xF) * 17;
    }

    return ftxui::Color::RGB(static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b));
  }
}
