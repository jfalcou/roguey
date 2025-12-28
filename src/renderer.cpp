//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================

#include "renderer.hpp"
#include <algorithm>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/terminal.hpp>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;
using namespace ftxui;

// --- Helper: Parse Hex Color ---
// Supports #RRGGBB and #RGB formats
static Color parse_hex_color(std::string const& hex_str)
{
  if (hex_str.empty() || hex_str[0] != '#') return Color::White;

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

  return Color::RGB(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
}

Renderer::Renderer() {}

Renderer::~Renderer() {}

void Renderer::load_colors(sol::state& lua)
{
  sol::table table = lua["game_colors"];
  if (!table.valid()) return;

  // Iterate over the Lua table
  for (auto const& [key, val] : table.as<std::map<std::string, sol::object>>())
  {
    ThemeStyle style;

    // Case 1: Value is a simple string "#RRGGBB" (Foreground only)
    if (val.is<std::string>())
    {
      style.fg = parse_hex_color(val.as<std::string>());
      style.has_bg = false;
    }
    // Case 2: Value is a table { fg="#...", bg="#..." }
    else if (val.is<sol::table>())
    {
      sol::table t = val.as<sol::table>();

      // Foreground
      std::string fg_str = t["fg"].get_or<std::string>("#FFFFFF");
      style.fg = parse_hex_color(fg_str);

      // Background (Optional)
      sol::optional<std::string> bg_str = t["bg"];
      if (bg_str)
      {
        style.bg = parse_hex_color(bg_str.value());
        style.has_bg = true;
      }
    }

    style_cache[key] = style;

    // Inject key back into Lua so scripts can reference 'ui_gold' etc.
    lua[key] = key;
  }
}

Decorator Renderer::get_style(std::string const& name) const
{
  auto it = style_cache.find(name);
  if (it != style_cache.end()) { return it->second.decorator(); }

  // Default fallback
  return color(Color::White);
}

void Renderer::animate_projectile(int x, int y, char glyph, std::string const& color)
{
  // Stub
}

Element Renderer::draw_log(MessageLog const& log)
{
  Elements list;
  int start_index = std::max(0, (int)log.messages.size() - 6);

  for (size_t i = start_index; i < log.messages.size(); ++i)
  {
    list.push_back(text(log.messages[i].text) | get_style(log.messages[i].color));
  }

  return vbox(std::move(list)) | size(HEIGHT, EQUAL, 6);
}

Element Renderer::render_dungeon(Dungeon const& map,
                                 Registry const& reg,
                                 MessageLog const& log,
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

  Position p = {0, 0};
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
          Position ep = reg.positions.at(id);
          if (ep.x == wx && ep.y == wy)
          {
            if (id == player_id || map.visible_tiles.count({wx, wy}))
            {
              std::string g(1, r.glyph);
              // Now uses get_style which supports BG
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

Element Renderer::render_inventory(std::vector<ItemTag> const& inventory, MessageLog const& log)
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

Element Renderer::render_stats(Registry const& reg, int player_id, std::string player_name, MessageLog const& log)
{
  if (!reg.stats.contains(player_id)) return text("Error: No Stats");

  auto s = reg.stats.at(player_id);

  return window(text(" Stats ") | get_style("ui_border"),
                vbox({filler(),
                      vbox({text("Name:   " + player_name), text("Class:  " + s.archetype),
                            text("Level:  " + std::to_string(s.level)), text("XP:     " + std::to_string(s.xp)),
                            text("HP:     " + std::to_string(s.hp) + " / " + std::to_string(s.max_hp)),
                            text("Mana:   " + std::to_string(s.mana) + " / " + std::to_string(s.max_mana)),
                            text("Damage: " + std::to_string(s.damage)), text("FOV:    " + std::to_string(s.fov_range)),
                            text("Gold:   " + std::to_string(s.gold))}) |
                        center,
                      filler(), separator(), text("[C/ESC] Close") | center, separator(), draw_log(log)})) |
         flex;
}

Element Renderer::render_help(MessageLog const& log, std::string const& help_text)
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

Element Renderer::render_character_creation(std::string const& current_name, MessageLog const& log)
{
  return window(text(" Create Your Hero ") | get_style("ui_border"),
                vbox({filler(), text("Enter your name:") | bold | get_style("ui_emphasis") | center,
                      text(current_name + "_") | bold | center | get_style("ui_gold"), filler(), separator(),
                      draw_log(log)})) |
         get_style("ui_border") | flex;
}

Element Renderer::render_class_selection(std::vector<std::string> const& classes, int selection, MessageLog const& log)
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

Element Renderer::render_game_over(MessageLog const& log)
{
  return window(text(" GAME OVER ") | get_style("ui_failure"),
                vbox({filler(), text(" !!! YOU DIED !!! ") | bold | center | get_style("ui_failure"),
                      text("Press 'r' to Restart, 'q' to Quit") | center, filler(), separator(), draw_log(log)})) |
         flex;
}

Element Renderer::render_victory(MessageLog const& log)
{
  return window(text(" VICTORY ") | get_style("ui_gold"),
                vbox({filler(), text(" !!! VICTORY !!! ") | bold | center | get_style("ui_gold"),
                      text("Press 'c' to Continue, 'q' to Quit") | center, filler(), separator(), draw_log(log)})) |
         flex;
}
