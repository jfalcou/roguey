//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <string>

namespace roguey
{
  struct ThemeStyle
  {
    ftxui::Color fg = ftxui::Color::White;
    ftxui::Color bg = ftxui::Color::Black;
    bool has_bg = false;

    ftxui::Decorator decorator() const
    {
      auto d = ftxui::color(fg);
      if (has_bg) d = d | ftxui::bgcolor(bg);
      return d;
    }
  };

  ftxui::Color parse_hex_color(std::string const& hex_str);
}
