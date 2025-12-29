//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "renderer.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>
#include <iostream>
#include <thread>

namespace roguey
{
  namespace fs = std::filesystem;
  using namespace ftxui;

  void renderer::load_config(sol::state& lua)
  {
    // 1. Load Colors
    sol::table colors = lua["game_colors"];
    if (colors.valid())
    {
      for (auto const& [key, val] : colors.as<std::map<std::string, sol::object>>())
      {
        theme_style style;
        if (val.is<std::string>())
        {
          std::string hex = val.as<std::string>();
          style.fg = parse_hex_color(hex);
          style.ansi_fg = hex_to_ansi(hex);
          style.has_bg = false;
        }
        else if (val.is<sol::table>())
        {
          sol::table t = val.as<sol::table>();
          std::string fg_str = t["fg"].get_or<std::string>("#FFFFFF");

          style.fg = parse_hex_color(fg_str);
          style.ansi_fg = hex_to_ansi(fg_str);

          sol::optional<std::string> bg_str = t["bg"];
          if (bg_str)
          {
            style.bg = parse_hex_color(bg_str.value());
            style.has_bg = true;
          }
        }
        style_cache[key] = style;
        // Expose keys back to Lua for scripts to use (e.g. "ui_gold")
        lua[key] = key;
      }
    }

    // 2. Load Speed Thresholds
    sol::table speeds = lua["speed_thresholds"];
    speed_thresholds.clear();
    if (speeds.valid())
    {
      for (auto const& [key, val] : speeds.as<std::map<int, sol::table>>())
      {
        SpeedThreshold st;
        st.limit = val.get_or("limit", 10);
        st.label = val.get_or<std::string>("label", "Unknown");
        st.color = val.get_or<std::string>("color", "ui_default");
        speed_thresholds.push_back(st);
      }
      // Ensure they are sorted by limit so we can find the first match
      std::sort(speed_thresholds.begin(), speed_thresholds.end(),
                [](SpeedThreshold const& a, SpeedThreshold const& b) { return a.limit < b.limit; });
    }
  }

  Decorator renderer::get_style(std::string const& name) const
  {
    auto it = style_cache.find(name);
    if (it != style_cache.end()) { return it->second.decorator(); }
    return color(Color::White);
  }

  Element renderer::draw_log(message_log const& log)
  {
    Elements list;
    int start_index = std::max(0, (int)log.messages.size() - 6);

    for (size_t i = start_index; i < log.messages.size(); ++i)
    {
      list.push_back(text(log.messages[i].text) | get_style(log.messages[i].color));
    }

    return vbox(std::move(list)) | size(HEIGHT, EQUAL, 6);
  }

  Element renderer::render_dungeon(dungeon const& map,
                                   registry const& reg,
                                   message_log const& log,
                                   int player_id,
                                   int depth,
                                   std::string const& title,
                                   std::string const& wall_color,
                                   std::string const& floor_color)
  {
    auto term_size = Terminal::Size();
    int overhead_y = 11;
    int overhead_x = 2;

    int view_w = term_size.dimx - overhead_x;
    int view_h = term_size.dimy - overhead_y;

    if (view_w < 10) view_w = 10;
    if (view_h < 5) view_h = 5;

    position p = {0, 0};
    if (reg.positions.count(player_id)) p = reg.positions.at(player_id);

    int cam_x = std::clamp(p.x - view_w / 2, 0, std::max(0, map.width - view_w));
    int cam_y = std::clamp(p.y - view_h / 2, 0, std::max(0, map.height - view_h));

    last_cam_x = cam_x;
    last_cam_y = cam_y;

    Elements grid_rows;

    for (int y = 0; y < view_h; ++y)
    {
      Elements row_cells;
      int wy = cam_y + y;

      for (int x = 0; x < view_w; ++x)
      {
        int wx = cam_x + x;

        if (wx >= map.width || wy >= map.height)
        {
          row_cells.push_back(text(" "));
          continue;
        }

        bool entity_drawn = false;
        for (auto const& [id, r] : reg.renderables)
        {
          if (reg.positions.count(id))
          {
            position ep = reg.positions.at(id);
            if (ep.x == wx && ep.y == wy)
            {
              if (id == player_id || map.visible_tiles.count({wx, wy}))
              {
                std::string g(1, r.glyph);
                row_cells.push_back(text(g) | get_style(r.color));
                entity_drawn = true;
                break;
              }
            }
          }
        }

        if (entity_drawn) continue;

        if (map.visible_tiles.count({wx, wy}))
        {
          char tile = map.grid(wx, wy);
          std::string c_key = (tile == '#') ? wall_color : floor_color;
          row_cells.push_back(text(std::string(1, tile)) | get_style(c_key));
        }
        else if (map.explored(wx, wy))
        {
          row_cells.push_back(text(std::string(1, map.grid(wx, wy))) | get_style("ui_hidden"));
        }
        else { row_cells.push_back(text(" ")); }
      }
      grid_rows.push_back(hbox(std::move(row_cells)));
    }

    auto s = reg.stats.at(player_id);
    auto status_line =
      hbox({text("HP: "), text(std::to_string(s.hp) + "/" + std::to_string(s.max_hp)) | get_style("ui_hp"),
            text(" | MP: "), text(std::to_string(s.mana) + "/" + std::to_string(s.max_mana)) | get_style("ui_mp"),
            text(" | Depth: " + std::to_string(depth))}) |
      center | get_style("ui_default");

    return window(text(" " + title + " ") | get_style("ui_border"),
                  vbox({vbox(std::move(grid_rows)) | flex, separator(), status_line, separator(), draw_log(log)})) |
           get_style("ui_border") | flex;
  }

  Element renderer::render_inventory(std::vector<item_tag> const& inventory, message_log const& log)
  {
    Elements items;
    if (inventory.empty()) items.push_back(text("(Empty)") | center);
    else
    {
      for (size_t i = 0; i < inventory.size(); ++i)
      {
        items.push_back(text("[" + std::to_string(i + 1) + "] " + inventory[i].name) | center);
      }
    }

    return window(text(" Inventory ") | get_style("ui_border"),
                  vbox({filler(), vbox(std::move(items)) | center, filler(), separator(),
                        text("[1-9] Use Item | [I/ESC] Close") | center, separator(), draw_log(log)})) |
           flex;
  }

  Element renderer::render_stats(registry const& reg, int player_id, std::string player_name, message_log const& log)
  {
    if (!reg.stats.contains(player_id)) return text("Error: No Stats");

    auto s = reg.stats.at(player_id);

    // Determine Speed Label using loaded configuration
    std::string speed_str = "Unknown";
    std::string speed_color = "ui_default";

    for (auto const& threshold : speed_thresholds)
    {
      if (s.action_delay <= threshold.limit)
      {
        speed_str = threshold.label;
        speed_color = threshold.color;
        break;
      }
    }

    return window(
             text(" Stats ") | get_style("ui_border"),
             vbox(
               {filler(),
                vbox({hbox({text("Name:   "), text(player_name) | get_style("ui_emphasis")}) | center,
                      hbox({text("Class:  "), text(s.archetype) | get_style("ui_emphasis")}) | center, text(" "),

                      hbox({text("Level:  "), text(std::to_string(s.level)) | get_style("ui_gold")}) | center,
                      hbox({text("XP:     "), text(std::to_string(s.xp)) | get_style("ui_text")}) | center, text(" "),

                      hbox({text("HP:     "),
                            text(std::to_string(s.hp) + " / " + std::to_string(s.max_hp)) | get_style("ui_hp")}) |
                        center,
                      hbox({text("Mana:   "),
                            text(std::to_string(s.mana) + " / " + std::to_string(s.max_mana)) | get_style("ui_mp")}) |
                        center,
                      text(" "),

                      hbox({text("Damage: "), text(std::to_string(s.damage)) | get_style("ui_failure")}) | center,
                      hbox({text("Speed:  "), text(speed_str) | get_style(speed_color)}) | center,
                      hbox({text("FOV:    "), text(std::to_string(s.fov_range)) | get_style("ui_text")}) | center,
                      text(" "),

                      hbox({text("Gold:   "), text(std::to_string(s.gold)) | get_style("ui_gold")}) | center}) |
                  center,
                filler(), separator(), text("[C/ESC] Close") | center, separator(), draw_log(log)})) |
           flex;
  }

  Element renderer::render_help(message_log const& log, std::string const& help_text)
  {
    Elements lines;
    std::stringstream ss(help_text);
    std::string line;
    while (std::getline(ss, line)) { lines.push_back(text(line) | flex); }

    return window(text(" HELP ") | get_style("ui_border"),
                  vbox({filler(), vbox(std::move(lines)) | center, filler(), separator(), text("[ESC] Back") | center,
                        separator(), draw_log(log)})) |
           flex;
  }

  Element renderer::render_character_creation(std::string const& current_name, message_log const& log)
  {
    return window(text(" Create Your Hero ") | get_style("ui_border"),
                  vbox({filler(), text("Enter your name:") | bold | get_style("ui_emphasis") | center,
                        text(current_name + "_") | bold | center | get_style("ui_gold"), filler(), separator(),
                        draw_log(log)})) |
           get_style("ui_border") | flex;
  }

  Element renderer::render_class_selection(std::vector<std::string> const& classes,
                                           int selection,
                                           message_log const& log)
  {
    Elements options;
    for (size_t i = 0; i < classes.size(); ++i)
    {
      std::string name = fs::path(classes[i]).stem().string();
      if ((int)i == selection) { options.push_back(text("[ " + name + " ]") | bold | get_style("ui_gold") | center); }
      else { options.push_back(text("  " + name + "  ") | center); }
    }

    return window(
             text(" Select Class ") | get_style("ui_border"),
             vbox({filler(), text("Choose your path:") | center | get_style("ui_text"),
                   vbox(std::move(options) | get_style("ui_text")) | center, filler(), separator(), draw_log(log)})) |
           get_style("ui_border") | flex;
  }

  Element renderer::render_game_over(message_log const& log)
  {
    return window(text(" GAME OVER ") | get_style("ui_failure"),
                  vbox({filler(), text(" !!! YOU DIED !!! ") | bold | center | get_style("ui_failure"),
                        text("Press 'r' to Restart, 'q' to Quit") | center, filler(), separator(), draw_log(log)})) |
           flex;
  }

  Element renderer::render_victory(message_log const& log)
  {
    return window(text(" VICTORY ") | get_style("ui_gold"),
                  vbox({filler(), text(" !!! VICTORY !!! ") | bold | center | get_style("ui_gold"),
                        text("Press 'c' to Continue, 'q' to Quit") | center, filler(), separator(), draw_log(log)})) |
           flex;
  }
}
