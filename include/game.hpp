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
#include "state_machine.hpp"
#include "systems.hpp"
#include <ftxui/component/component.hpp>
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

    void stop();

    // State Transition Helper
    template<typename T> void set_state(T&& new_state) { machine.transition_to(std::forward<T>(new_state)); }

    // Public Data
    void set_menu_lock(int frames) { menu_lock = frames; }

    void reset(bool full_reset, std::string level_script = "");

    dungeon map;
    registry reg;
    renderer renderer;
    script_engine scripts;
    message_log log;
    std::vector<item_tag> inventory;

    // Persistent Player State
    int last_dx = 1;
    int last_dy = 0;

    // Level Management
    int depth = 1;
    std::string current_level_script;

    // Input Control
    bool has_buffered_event = false;
    ftxui::Event buffered_event;
    int menu_lock = 0;

  private:
    void spawn_item(int x, int y, std::string script_path);
    bool spawn_monster(int x, int y, std::string script_path);

    bool debug_mode;
    bool running;

    state_machine machine;

    std::random_device random_bits;
    std::mt19937 random_generator;
  };
}
