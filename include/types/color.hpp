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
  struct theme_style
  {
    ftxui::Color fg = ftxui::Color::White;
    ftxui::Color bg = ftxui::Color::Black;
    std::string ansi_fg = "\033[37m"; // Default to white
    bool has_bg = false;

    ftxui::Decorator decorator() const
    {
      auto d = ftxui::color(fg);
      if (has_bg) d = d | ftxui::bgcolor(bg);
      return d;
    }
  };

  ftxui::Color parse_hex_color(std::string const& hex_str);
  std::string hex_to_ansi(std::string const& hex);
}
