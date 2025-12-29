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
  Game::Game(bool debug) : map(80, 20), debug_mode(debug), scripts{"scripts/game.lua"}, random_generator(random_bits())
  {
    if (!scripts.is_valid) { exit(1); }

    renderer.load_colors(scripts.lua);
    scripts.lua.set_function("roll", [prng = &random_generator](std::string const& dice) { return roll(dice, *prng); });

    is_setup = true;
    setup_step = 0;
    input_buffer = "";
    running = true;
  }

  Game::~Game() {}

  ftxui::Element Game::render_ui()
  {
    if (is_setup)
    {
      if (setup_step == 0) return renderer.render_character_creation(input_buffer, log);
      if (setup_step == 1) return renderer.render_class_selection(scripts.class_templates, class_selection, log);
    }

    if (reg.stats.contains(reg.player_id) && reg.stats[reg.player_id].hp <= 0)
    {
      return renderer.render_game_over(log);
    }

    if (reg.boss_id != 0 && !reg.positions.contains(reg.boss_id)) { return renderer.render_victory(log); }

    if (state == game_state::Dungeon || state == game_state::Animating)
    {
      // RESTORED: Fetch level config for dynamic Title and Colors
      sol::table config = scripts.lua["get_level_config"](depth);

      std::string level_name = config["name"].get_or<std::string>("Unknown");
      std::string wall_color = config["wall_color"].get_or<std::string>("asset_wall");
      std::string floor_color = config["floor_color"].get_or<std::string>("asset_floor");

      return renderer.render_dungeon(map, reg, log, reg.player_id, depth, level_name, wall_color, floor_color);
    }
    else if (state == game_state::Inventory) { return renderer.render_inventory(inventory, log); }
    else if (state == game_state::Stats) { return renderer.render_stats(reg, reg.player_id, reg.player_name, log); }
    else if (state == game_state::Help)
    {
      std::string txt = scripts.lua["help_text"].get_or<std::string>("No help text defined.");
      return renderer.render_help(log, txt);
    }

    return ftxui::text("Unknown State");
  }

  bool Game::on_event(ftxui::Event event)
  {
    // Handle Animation Tick
    if (event == ftxui::Event::Special({0}))
    {
      if (state == game_state::Animating)
      {
        update_animation();
        return true; // Request Redraw
      }
      return false; // No redraw needed
    }

    // Ignore input during animation
    if (state == game_state::Animating) return false;

    if (is_setup)
    {
      handle_setup_input(event);
      return true;
    }

    if (reg.stats.contains(reg.player_id) && reg.stats[reg.player_id].hp <= 0)
    {
      handle_game_over_input(event);
      return true;
    }

    if (reg.boss_id != 0 && !reg.positions.contains(reg.boss_id))
    {
      handle_victory_input(event);
      return true;
    }

    if (state == game_state::Dungeon) handle_dungeon_input(event);
    else if (state == game_state::Inventory) handle_inventory_input(event);
    else
    {
      if (event == ftxui::Event::Escape || event == ftxui::Event::Character('c')) { state = game_state::Dungeon; }
    }
    return true;
  }

  void Game::handle_setup_input(ftxui::Event event)
  {
    if (setup_step == 0)
    {
      if (event.is_character()) { input_buffer += event.character(); }
      else if (event == ftxui::Event::Backspace)
      {
        if (!input_buffer.empty()) input_buffer.pop_back();
      }
      else if (event == ftxui::Event::Return)
      {
        if (!input_buffer.empty())
        {
          reg.player_name = input_buffer;
          log.add("Welcome, " + reg.player_name + "!", "ui_text");
          setup_step = 1;
        }
      }
    }
    else if (setup_step == 1)
    {
      if (event == ftxui::Event::ArrowUp && class_selection > 0) class_selection--;
      if (event == ftxui::Event::ArrowDown && class_selection < (int)scripts.class_templates.size() - 1)
        class_selection++;
      if (event == ftxui::Event::Return || event == ftxui::Event::Character(' '))
      {
        reg.player_class_script = scripts.class_templates[class_selection];

        is_setup = false;

        std::string first_level =
          scripts.configuration["start_level"].get_or<std::string>("scripts/levels/dungeon.lua");
        std::string init_msg = scripts.configuration["initial_log_message"].get_or<std::string>("Welcome!");

        log.add(init_msg);
        reset(true, first_level);
      }
    }
  }

  void Game::handle_dungeon_input(ftxui::Event event)
  {
    int dx = 0, dy = 0;
    bool acted = false;

    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q'))
    {
      running = false;
      return;
    }

    if (event == ftxui::Event::ArrowUp)
    {
      dy = -1;
      acted = true;
    }
    else if (event == ftxui::Event::ArrowDown)
    {
      dy = +1;
      acted = true;
    }
    else if (event == ftxui::Event::ArrowLeft)
    {
      dx = -1;
      acted = true;
    }
    else if (event == ftxui::Event::ArrowRight)
    {
      dx = +1;
      acted = true;
    }
    else if (event == ftxui::Event::Character('i'))
    {
      state = game_state::Inventory;
      return;
    }
    else if (event == ftxui::Event::Character('c'))
    {
      state = game_state::Stats;
      return;
    }
    else if (event == ftxui::Event::Escape)
    {
      state = game_state::Help;
      return;
    }
    else if (event == ftxui::Event::Character('f'))
    {
      Systems::cast_fireball(reg, map, last_dx, last_dy, log, scripts.lua);
      acted = true;
    }

    if (acted)
    {
      if (dx != 0 || dy != 0)
      {
        last_dx = dx;
        last_dy = dy;
        Position& p = reg.positions[reg.player_id];
        EntityID target = Systems::get_entity_at(reg, p.x + dx, p.y + dy);

        if (target && reg.stats.contains(target)) { Systems::attack(reg, reg.player_id, target, log, scripts.lua); }
        else if (map.is_walkable(p.x + dx, p.y + dy))
        {
          p.x += dx;
          p.y += dy;
          if (target && reg.items.contains(target))
          {
            if (reg.items[target].type == ItemType::Stairs)
            {
              reset(false, reg.items[target].name);
              return;
            }

            scripts.load_script(reg.items[target].script);

            if (scripts.lua["on_pick"](reg.stats[reg.player_id], log)) { inventory.push_back(reg.items[target]); }
            reg.destroy_entity(target);
          }
        }
      }

      // Check if animations need to start
      if (!reg.projectiles.empty()) { state = game_state::Animating; }
      else
      {
        // No animations, finish turn immediately
        Systems::move_monsters(reg, map, log, scripts.lua);
        map.update_fov(reg.positions[reg.player_id].x, reg.positions[reg.player_id].y,
                       reg.stats[reg.player_id].fov_range);
      }
    }
  }

  void Game::update_animation()
  {
    // Move all projectiles one step
    Systems::update_projectiles(reg, map, log, scripts.lua);

    // If all projectiles are gone, finish the turn
    if (reg.projectiles.empty())
    {
      state = game_state::Dungeon;
      Systems::move_monsters(reg, map, log, scripts.lua);
      map.update_fov(reg.positions[reg.player_id].x, reg.positions[reg.player_id].y,
                     reg.stats[reg.player_id].fov_range);
    }
  }

  void Game::handle_inventory_input(ftxui::Event event)
  {
    if (event == ftxui::Event::Escape || event == ftxui::Event::Character('i'))
    {
      state = game_state::Dungeon;
      return;
    }

    if (event.is_character())
    {
      char ch = event.character()[0];
      if (ch >= '1' && ch <= '9')
      {
        std::size_t idx = ch - '1';
        if (idx < inventory.size())
        {
          auto& item = inventory[idx];
          if (scripts.load_script(item.script))
          {
            sol::protected_function on_use = scripts.lua["on_use"];
            auto use_res = on_use(reg.stats[reg.player_id], log);
            if (use_res.valid())
            {
              inventory.erase(inventory.begin() + idx);
              state = game_state::Dungeon;
            }
            else
            {
              sol::error err = use_res;
              log.add("Script Error: " + std::string(err.what()), "ui_emphasis");
            }
          }
        }
      }
    }
  }

  void Game::handle_game_over_input(ftxui::Event event)
  {
    if (event == ftxui::Event::Character('r') || event == ftxui::Event::Character('R'))
    {
      sol::table config = scripts.lua["get_start_config"]();
      std::string start_level = config["start_level"];
      log.add(config["initial_log_message"]);
      reset(true, start_level);
    }
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q')) { running = false; }
  }

  void Game::handle_victory_input(ftxui::Event event)
  {
    if (event == ftxui::Event::Character('c') || event == ftxui::Event::Character('C'))
    {
      scripts.load_script(current_level_script);
      reset(false, scripts.lua["get_next_level"](depth));
    }
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q')) { running = false; }
  }

  void Game::spawn_item(int x, int y, std::string script_path)
  {
    if (!scripts.load_script(script_path)) return;
    EntityID id = reg.create_entity();
    reg.positions[id] = {x, y};
    sol::table data = scripts.lua["item_data"];
    std::string glyph_str = data["glyph"];

    std::string cid = data["color"].get_or<std::string>("item_gold");
    reg.renderables[id] = {glyph_str[0], cid};

    ItemType kind;
    std::string item_kind = data["kind"];
    if (item_kind == "consumable") kind = ItemType::Consumable;
    else if (item_kind == "gold") kind = ItemType::Gold;

    reg.items[id] = {kind, 0, data["name"], script_path};
    reg.names[id] = data["name"];
  }

  bool Game::spawn_monster(int x, int y, std::string script_path)
  {
    if (debug_mode) { log.add("Spawning: " + script_path, "ui_emphasis"); }

    if (!scripts.load_script(script_path))
    {
      if (debug_mode) log.add("Error loading: " + script_path, "ui_failure");
      return false;
    }

    EntityID id = reg.create_entity();
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
    reg.stats[id] = {s["type"], s["hp"], s["hp"], 0, 0, s["damage"], 0, 1, 0};

    std::string glyph_str = s["glyph"].get<std::string>();
    char glyph = glyph_str.empty() ? '?' : glyph_str[0];

    std::string cid = s["color"].get_or<std::string>("ui_default");
    reg.renderables[id] = {glyph, cid};

    std::string m_type = s["type"];
    std::string name = s["name"].get_or(fs::path(script_path).stem().string());
    reg.names[id] = name;

    if (m_type == "boss") reg.boss_id = id;

    return true;
  }

  void Game::reset(bool full_reset, std::string level_script)
  {
    Stats saved_stats;
    if (!full_reset)
    {
      saved_stats = reg.stats[reg.player_id];
      depth++;
    }
    else
    {
      depth = 1;
      inventory.clear();
      last_dx = 1;
      last_dy = 0;
    }
    if (!level_script.empty()) current_level_script = level_script;

    reg.clear();
    state = game_state::Dungeon;

    scripts.load_script(current_level_script);
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
      scripts.load_script(reg.player_class_script);
      sol::protected_function init_func = scripts.lua["get_init_stats"];
      sol::table s = init_func();
      reg.stats[reg.player_id] = {s["archetype"], s["hp"], s["hp"], s["mp"], s["mp"], s["damage"], 0, 1, 8};
    }
    else { reg.stats[reg.player_id] = saved_stats; }

    std::string next_level_path = scripts.lua["get_next_level"](depth);

    if (config["is_boss_level"].get_or(false))
    {
      spawn_monster(map.rooms.back().center().x, map.rooms.back().center().y, scripts.lua["get_boss_script"](depth));
    }

    EntityID stairs = reg.create_entity();
    reg.positions[stairs] = map.rooms.back().center();
    reg.renderables[stairs] = {'>', "ui_gold"};
    reg.items[stairs] = {ItemType::Stairs, 0, next_level_path, ""};
    reg.names[stairs] = "Stairs";

    sol::table monster_weights = scripts.lua["get_spawn_odds"](depth);
    sol::table item_weights = scripts.lua["get_loot_odds"](depth);

    std::map<std::string, int> spawn_counts;

    for (std::size_t i = 1; i < map.rooms.size() - 1; ++i)
    {
      Position c = map.rooms[i].center();
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
