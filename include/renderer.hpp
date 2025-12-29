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
#include <map>
#include <string>
#include <vector>

namespace roguey
{
  class renderer
  {
  public:
    renderer() {}

    ~renderer() {}

    void load_config(sol::state& lua);

    ftxui::Decorator get_style(std::string const& name) const;

    ftxui::Element render_dungeon(dungeon const& map,
                                  registry const& reg,
                                  message_log const& log,
                                  int player_id,
                                  int depth,
                                  std::string const& title,
                                  std::string const& wall_color,
                                  std::string const& floor_color);

    ftxui::Element render_inventory(std::vector<item_tag> const& inventory, message_log const& log);
    ftxui::Element render_stats(registry const& reg, int player_id, std::string player_name, message_log const& log);
    ftxui::Element render_help(message_log const& log, std::string const& help_text);

    ftxui::Element render_character_creation(std::string const& current_name, message_log const& log);
    ftxui::Element render_class_selection(std::vector<std::string> const& classes,
                                          int selection,
                                          message_log const& log);
    ftxui::Element render_game_over(message_log const& log);
    ftxui::Element render_victory(message_log const& log);

  private:
    ftxui::Element draw_log(message_log const& log);

    std::map<std::string, theme_style> style_cache;
    std::vector<SpeedThreshold> speed_thresholds; // Stores loaded speed config

    int last_cam_x = 0;
    int last_cam_y = 0;
  };
}
