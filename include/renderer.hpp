#pragma once
#include "dungeon.hpp"
#include "registry.hpp"
#include "script_engine.hpp"
#include "systems.hpp"
#include "types.hpp"
#include <string>
#include <vector>

class Renderer
{
public:
  int screen_width = 80;
  int screen_height = 24;

  Renderer();
  ~Renderer();

  void init_colors(ScriptEngine& scripts);
  void set_window_size(int w, int h);
  void clear_screen();
  void refresh_screen();
  void show_error(std::string const& msg);

  void animate_projectile(int x, int y, char glyph, ColorPair color);

  void draw_log(MessageLog const& log, int start_y, int max_row, int max_col);

  void draw_dungeon(Dungeon const& map,
                    Registry const& reg,
                    MessageLog const& log,
                    int player_id,
                    int depth,
                    int wall_color,
                    int floor_color,
                    std::string const& level_name);

  void draw_borders(int x, int y, int width, int height, std::string const& title = "", int separator_y = -1);

  void draw_inventory(std::vector<ItemTag> const& inventory, MessageLog const& log);
  void draw_stats(Registry const& reg, int player_id, std::string player_name, MessageLog const& log);

  // Added draw_help
  void draw_help(MessageLog const& log, std::string const& help_text);

  void draw_character_creation_header(MessageLog const& log);
  void draw_class_selection(std::vector<std::string> const& class_paths, int selection, MessageLog const& log);

  void draw_game_over(MessageLog const& log);
  void draw_victory(MessageLog const& log);

private:
  int clamp(int val, int min, int max);
};
