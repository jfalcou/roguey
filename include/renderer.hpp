//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include "dungeon.hpp"
#include "registry.hpp"
#include "script_engine.hpp"
#include "systems.hpp"
#include "types.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <map>
#include <string>
#include <vector>

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

class Renderer
{
public:
  Renderer();
  ~Renderer();

  void load_colors(sol::state& lua);

  ftxui::Decorator get_style(std::string const& name) const;

  ftxui::Element render_dungeon(Dungeon const& map,
                                Registry const& reg,
                                MessageLog const& log,
                                int player_id,
                                int depth,
                                std::string const& title,
                                std::string const& wall_color,
                                std::string const& floor_color);

  ftxui::Element render_inventory(std::vector<ItemTag> const& inventory, MessageLog const& log);
  ftxui::Element render_stats(Registry const& reg, int player_id, std::string player_name, MessageLog const& log);
  ftxui::Element render_help(MessageLog const& log, std::string const& help_text);

  ftxui::Element render_character_creation(std::string const& current_name, MessageLog const& log);
  ftxui::Element render_class_selection(std::vector<std::string> const& classes, int selection, MessageLog const& log);
  ftxui::Element render_game_over(MessageLog const& log);
  ftxui::Element render_victory(MessageLog const& log);

  void animate_projectile(int x, int y, char glyph, std::string const& color);

private:
  ftxui::Element draw_log(MessageLog const& log);
  std::map<std::string, ThemeStyle> style_cache;

  int last_cam_x = 0;
  int last_cam_y = 0;
};
