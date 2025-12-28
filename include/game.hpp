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
#include "renderer.hpp"
#include "script_engine.hpp"
#include "systems.hpp"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <random>
#include <vector>

namespace roguey
{
  class Game
  {
    Registry reg;
    Dungeon map;
    Renderer renderer;
    MessageLog log;
    ScriptEngine scripts;

    std::random_device random_bits;
    std::mt19937 random_generator;

    bool debug_mode = false;
    game_state state = game_state::Dungeon;
    int last_dx = 1, last_dy = 0;
    int depth = 1;

    std::string current_level_script;
    std::vector<ItemTag> inventory;

    bool running = true;
    bool is_setup = true;
    int setup_step = 0;
    std::string input_buffer;
    int class_selection = 0;

  public:
    Game(bool debug = false);
    ~Game();

    bool on_event(ftxui::Event event);
    ftxui::Element render_ui();

    bool is_running() const { return running; }

  private:
    void reset(bool full_reset, std::string level_script = "");
    bool spawn_monster(int x, int y, std::string script_path);
    void spawn_item(int x, int y, std::string script_path);
    void handle_dungeon_input(ftxui::Event event);
    void handle_inventory_input(ftxui::Event event);
    void handle_setup_input(ftxui::Event event);
    void handle_game_over_input(ftxui::Event event);
    void handle_victory_input(ftxui::Event event);
  };
}
