#pragma once
#include "dungeon.hpp"
#include "registry.hpp"
#include "renderer.hpp"
#include "script_engine.hpp"
#include "systems.hpp"
#include <vector>

class Game
{
  Registry reg;
  Dungeon map;
  MessageLog log;
  ScriptEngine scripts;
  Renderer renderer; // The new renderer

  bool running = true;
  bool debug_mode = false;
  game_state state = game_state::Dungeon;
  int last_dx = 1, last_dy = 0;
  int depth = 1;
  std::string current_level_script;
  std::vector<ItemTag> inventory;

public:
  Game(bool debug = false);
  ~Game();
  void run();

private:
  void reset(bool full_reset, std::string level_script = "");
  bool spawn_monster(int x, int y, std::string script_path);
  void spawn_item(int x, int y, std::string script_path);
  void process_input();
  void render();
  void handle_input_inventory(int ch);
  void get_player_setup();
  bool handle_game_over();
  bool handle_victory();
};
