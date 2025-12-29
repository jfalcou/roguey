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
#include <ftxui/component/component.hpp>
#include <optional>
#include <random>
#include <vector>

namespace roguey
{
  class game
  {
  public:
    game(bool debug = false);
    ~game();

    ftxui::Element render_ui();
    bool on_event(ftxui::Event event);

    bool is_running() const { return running; }

  private:
    void handle_setup_input(ftxui::Event event);
    void handle_dungeon_input(ftxui::Event event);
    void handle_inventory_input(ftxui::Event event);
    void handle_game_over_input(ftxui::Event event);
    void handle_victory_input(ftxui::Event event);

    bool update_animation(); // Returns true if redraw is needed

    void reset(bool full_reset, std::string level_script = "");
    void spawn_item(int x, int y, std::string script_path);
    bool spawn_monster(int x, int y, std::string script_path);

    bool debug_mode;
    bool running;

    // UI State
    bool is_setup;
    int setup_step;
    std::string input_buffer;
    int class_selection = 0;
    game_state state = game_state::Dungeon;

    // Input Control
    bool has_buffered_event = false;
    ftxui::Event buffered_event;
    int menu_lock = 0; // Debounce timer for UI transitions

    // Game Data
    dungeon map;
    registry reg;
    renderer renderer;
    script_engine scripts;
    message_log log;
    std::vector<item_tag> inventory;

    // Level Management
    int depth = 1;
    std::string current_level_script;

    // Input memory
    int last_dx = 1;
    int last_dy = 0;

    // Random Number Generation
    std::random_device random_bits;
    std::mt19937 random_generator;
  };
}
