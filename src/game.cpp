//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "dice.hpp"
#include "game.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <random>

namespace fs = std::filesystem;

namespace roguey
{
  game::game(bool debug) : debug_mode(debug), map(80, 20), scripts{"scripts/game.lua"}, random_generator(random_bits())
  {
    if (!scripts.is_valid) { exit(1); }

    renderer.load_config(scripts.lua);
    scripts.lua.set_function("roll", [prng = &random_generator](std::string const& dice) { return roll(dice, *prng); });

    running = true;
    buffered_event = ftxui::Event::Special({0});
  }

  game::~game() {}

  void game::stop()
  {
    running = false;
  }

  ftxui::Element game::render_ui()
  {
    return machine.render(*this);
  }

  bool game::on_event(ftxui::Event event)
  {
    if (menu_lock > 0) menu_lock--;
    return machine.on_event(*this, event);
  }

  void game::spawn_item(int x, int y, std::string script_path)
  {
    if (!systems::execute_script(scripts.lua, script_path, log)) return;

    auto data_opt = systems::try_get_table(scripts.lua, "item_data", log);
    if (!data_opt) return;
    sol::table data = *data_opt;

    entity_id id = reg.create_entity();
    reg.positions[id] = {x, y};

    std::string glyph_str = data["glyph"];
    std::string cid = data.get_or<std::string>("color", "item_gold");
    reg.renderables[id] = {glyph_str[0], cid};

    item_type kind;
    std::string item_kind = data["kind"];
    if (item_kind == "consumable") kind = item_type::Consumable;
    else if (item_kind == "gold") kind = item_type::Gold;

    reg.items[id] = {kind, 0, data["name"], script_path};
    reg.names[id] = data["name"];
  }

  bool game::spawn_monster(int x, int y, std::string script_path)
  {
    if (debug_mode) { log.add("Spawning: " + script_path, "ui_emphasis"); }

    if (!systems::execute_script(scripts.lua, script_path, log)) return false;

    entity_id id = reg.create_entity();
    reg.positions[id] = {x, y};
    reg.script_paths[id] = script_path;
    reg.monsters.push_back(id);

    sol::protected_function init_func = scripts.lua["get_init_stats"];
    auto result = init_func();

    if (!result.valid())
    {
      sol::error err = result;
      if (debug_mode) log.add("Lua Error: " + std::string(err.what()), "ui_failure");
      reg.destroy_entity(id);
      return false;
    }

    sol::table s = result;
    auto cfg = systems::parse_entity_config(s, fs::path(script_path).stem().string());

    int start_timer = std::uniform_int_distribution<>(0, cfg.stats.action_delay)(random_generator);
    cfg.stats.action_timer = start_timer;

    reg.stats[id] = cfg.stats;
    reg.renderables[id] = cfg.render;
    reg.names[id] = cfg.name;

    if (cfg.type == "boss") reg.boss_id = id;

    return true;
  }

  void game::reset(bool full_reset, std::string level_script)
  {
    stats saved_stats;
    if (!full_reset)
    {
      saved_stats = reg.stats[reg.player_id];
      saved_stats.action_timer = 0;
      depth++;
    }
    else
    {
      depth = 1;
      inventory.clear();
      last_dx = 1; // Reset direction on full reset
      last_dy = 0;
    }
    if (!level_script.empty()) current_level_script = level_script;

    // Critical Fixes for Level Transition
    reg.clear();
    reg.boss_id = 0;            // Ensure no lingering boss ID causes instant victory
    has_buffered_event = false; // Clear input buffer to prevent accidental moves

    systems::execute_script(scripts.lua, current_level_script, log);
    sol::table config = scripts.lua["get_level_config"](depth);

    map.width = config["width"];
    map.height = config["height"];
    map.generate(random_generator);

    reg.player_id = reg.create_entity();
    reg.positions[reg.player_id] = map.rooms[0].center();

    reg.renderables[reg.player_id] = {'@', "entity_player"};
    reg.names[reg.player_id] = reg.player_name;

    if (full_reset)
    {
      systems::execute_script(scripts.lua, reg.player_class_script, log);
      sol::protected_function init_func = scripts.lua["get_init_stats"];
      sol::table s = init_func();

      auto cfg = systems::parse_entity_config(s, "Hero");
      reg.stats[reg.player_id] = cfg.stats;
    }
    else { reg.stats[reg.player_id] = saved_stats; }

    std::string next_level_path = scripts.lua["get_next_level"](depth);

    if (config["is_boss_level"].get_or(false))
    {
      spawn_monster(map.rooms.back().center().x, map.rooms.back().center().y, scripts.lua["get_boss_script"](depth));
    }

    entity_id stairs = reg.create_entity();
    reg.positions[stairs] = map.rooms.back().center();
    reg.renderables[stairs] = {'>', "ui_gold"};
    reg.items[stairs] = {item_type::Stairs, 0, next_level_path, ""};
    reg.names[stairs] = "Stairs";

    sol::table monster_weights = scripts.lua["get_spawn_odds"](depth);
    sol::table item_weights = scripts.lua["get_loot_odds"](depth);

    std::map<std::string, int> spawn_counts;

    for (std::size_t i = 1; i < map.rooms.size() - 1; ++i)
    {
      position c = map.rooms[i].center();
      int roll = std::uniform_int_distribution<>(0, 10)(random_generator);
      if (roll < 3)
      {
        std::string path = scripts.pick_from_weights(item_weights, random_generator);
        if (!path.empty()) spawn_item(c.x, c.y, path);
      }
      else if (roll < 7)
      {
        std::string path = scripts.pick_from_weights(monster_weights, random_generator);
        if (!path.empty())
        {
          if (spawn_monster(c.x, c.y, path)) { spawn_counts[fs::path(path).stem().string()]++; }
        }
      }
    }

    if (debug_mode)
    {
      std::string debug_msg = "Debug Spawn: ";
      for (auto const& [name, count] : spawn_counts) { debug_msg += name + " x" + std::to_string(count) + " "; }
      log.add(debug_msg, "ui_gold");
    }

    map.update_fov(reg.positions[reg.player_id].x, reg.positions[reg.player_id].y, 8);
  }
}
