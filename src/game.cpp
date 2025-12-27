#include "game.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <ncurses.h>
#include <random>

namespace fs = std::filesystem;

Game::Game(bool debug) : map(80, 20), debug_mode(debug)
{
  scripts.init_lua();
  scripts.lua.new_usertype<MessageLog>("Log", "add",
                                       [](MessageLog& l, std::string msg) { l.add(msg, ColorPair::Default); });

  if (!scripts.load_script("scripts/game.lua"))
  {
    renderer.show_error("Failed to load 'scripts/game.lua'.\nCheck that the file exists and is valid.");
    exit(1);
  }

  renderer.init_colors(scripts);

  sol::protected_function start_cfg_func = scripts.lua["get_start_config"];
  sol::table start_config = start_cfg_func();

  int w = start_config["window_width"].get_or(80);
  int h = start_config["window_height"].get_or(24);
  renderer.set_window_size(w, h);

  scripts.discover_assets();
  get_player_setup();

  std::string first_level = start_config["start_level"];
  log.add(start_config["initial_log_message"]);
  reset(true, first_level);
}

Game::~Game() {}

void Game::get_player_setup()
{
  echo();
  curs_set(1);

  while (true)
  {
    renderer.draw_character_creation_header(log);

    char name_buf[32];
    getnstr(name_buf, 31);
    std::string input(name_buf);

    bool invalid = input.empty() || std::all_of(input.begin(), input.end(),
                                                [](unsigned char c) { return std::isspace(c) || !std::isprint(c); });

    if (!invalid)
    {
      reg.player_name = input;
      log.add("Welcome, " + reg.player_name + "!", ColorPair::Gold);
      break;
    }
    else { log.add("Invalid name. Please try again.", ColorPair::Orc); }
  }

  noecho();
  curs_set(0);

  int selection = 0;
  while (true)
  {
    renderer.draw_class_selection(scripts.class_templates, selection, log);

    int ch = getch();
    if (ch == KEY_UP && selection > 0) selection--;
    if (ch == KEY_DOWN && selection < (int)scripts.class_templates.size() - 1) selection++;
    if (ch == 10 || ch == 13 || ch == ' ')
    {
      reg.player_class_script = scripts.class_templates[selection];
      break;
    }
  }
}

void Game::spawn_item(int x, int y, std::string script_path)
{
  if (!scripts.load_script(script_path)) return;
  EntityID id = reg.create_entity();
  reg.positions[id] = {x, y};
  sol::table data = scripts.lua["item_data"];
  std::string glyph_str = data["glyph"];
  reg.renderables[id] = {glyph_str[0], static_cast<ColorPair>(data["color"].get<int>())};
  reg.items[id] = {ItemType::Consumable, 0, data["name"], script_path};
  reg.names[id] = data["name"];
}

bool Game::spawn_monster(int x, int y, std::string script_path)
{
  if (debug_mode) { log.add("Spawning: " + script_path, ColorPair::Orc); }
  if (!scripts.load_script(script_path))
  {
    // Log the error to the in-game log so you can see it!
    log.add("Error: Could not load " + script_path, ColorPair::Orc);
    return false;
  }

  EntityID id = reg.create_entity();
  reg.positions[id] = {x, y};
  reg.script_paths[id] = script_path;
  reg.monsters.push_back(id);

  sol::protected_function init_func = scripts.lua["get_init_stats"];
  auto result = init_func();

  // Check if the Lua function execution succeeded
  if (!result.valid())
  {
    sol::error err = result;
    log.add("Lua Error in " + script_path + ": " + std::string(err.what()), ColorPair::Orc);
    reg.destroy_entity(id); // Cleanup
    return false;
  }

  sol::table s = result;
  reg.stats[id] = {s["hp"], s["hp"], 0, 0, s["damage"], 0, 1, 0};

  std::string glyph_str = s["glyph"].get<std::string>();
  char glyph = glyph_str.empty() ? '?' : glyph_str[0];

  int cid = s["color"].get_or(0); // Use get_or to prevent crash if color is missing
  reg.renderables[id] = {glyph, static_cast<ColorPair>(cid == 0 ? 1 : cid)};

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

  reg.positions.clear();
  reg.renderables.clear();
  reg.items.clear();
  reg.stats.clear();
  reg.script_paths.clear();
  reg.monster_types.clear();
  reg.monsters.clear();
  reg.names.clear();
  reg.boss_id = 0;
  reg.next_id = 1;
  state = game_state::Dungeon;

  scripts.load_script(current_level_script);
  sol::table config = scripts.lua["get_level_config"](depth);

  map.width = config["width"];
  map.height = config["height"];
  map.generate();

  reg.player_id = reg.create_entity();
  reg.positions[reg.player_id] = map.rooms[0].center();
  reg.renderables[reg.player_id] = {'@', ColorPair::Player};
  reg.names[reg.player_id] = reg.player_name;

  if (full_reset)
  {
    scripts.load_script(reg.player_class_script);
    sol::protected_function init_func = scripts.lua["get_init_stats"];
    sol::table s = init_func();
    reg.stats[reg.player_id] = {s["hp"], s["hp"], s["mp"], s["mp"], s["damage"], 0, 1, 8};
  }
  else { reg.stats[reg.player_id] = saved_stats; }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::string next_level_path = scripts.lua["get_next_level"](depth);

  if (config["is_boss_level"].get_or(false))
  {
    spawn_monster(map.rooms.back().center().x, map.rooms.back().center().y, scripts.lua["get_boss_script"](depth));
  }
  else
  {
    EntityID stairs = reg.create_entity();
    reg.positions[stairs] = map.rooms.back().center();
    reg.renderables[stairs] = {'>', ColorPair::Gold};
    reg.items[stairs] = {ItemType::Stairs, 0, next_level_path, ""};
    reg.names[stairs] = "Stairs";
  }

  sol::table monster_weights = scripts.lua["get_spawn_odds"](depth);
  sol::table item_weights = scripts.lua["get_loot_odds"](depth);

  std::map<std::string, int> spawn_counts;
  for (size_t i = 1; i < map.rooms.size() - 1; ++i)
  {
    Position c = map.rooms[i].center();
    int roll = std::uniform_int_distribution<>(0, 10)(gen);
    if (roll < 3)
    {
      std::string path = scripts.pick_from_weights(item_weights, gen);
      if (!path.empty()) spawn_item(c.x, c.y, path);
    }
    else if (roll < 7)
    {
      std::string path = scripts.pick_from_weights(monster_weights, gen);
      if (!path.empty())
      {
        if (spawn_monster(c.x, c.y, path)) spawn_counts[fs::path(path).stem().string()]++;
      }
    }
  }

  if (debug_mode)
  {
    std::string debug_msg = "Debug Spawn: ";
    for (auto const& [name, count] : spawn_counts) { debug_msg += name + " x" + std::to_string(count) + " "; }
    log.add(debug_msg, ColorPair::Gold);
  }
  map.update_fov(reg.positions[reg.player_id].x, reg.positions[reg.player_id].y, 8);
}

void Game::run()
{
  while (running)
  {
    if (reg.stats[reg.player_id].hp <= 0)
    {
      if (handle_game_over())
      {
        sol::table config = scripts.lua["get_start_config"]();
        std::string start_level = config["start_level"];
        log.add(config["initial_log_message"]);
        reset(true, start_level);
        continue;
      }
      else break;
    }
    if (reg.boss_id != 0 && !reg.positions.contains(reg.boss_id))
    {
      if (handle_victory())
      {
        scripts.load_script(current_level_script);
        reset(false, scripts.lua["get_next_level"](depth));
        continue;
      }
      else break;
    }
    render();
    process_input();
  }
}

void Game::process_input()
{
  int ch = getch();
  if (ch == 'i' || ch == 'I')
  {
    state = (state == game_state::Inventory) ? game_state::Dungeon : game_state::Inventory;
    return;
  }
  if (ch == 'c' || ch == 'C')
  {
    state = (state == game_state::Stats) ? game_state::Dungeon : game_state::Stats;
    return;
  }
  if (ch == 27) // ESC handling
  {
    // If in Dungeon, go to Help. Else close whatever screen is open (Inv/Stats/Help)
    if (state == game_state::Dungeon) state = game_state::Help;
    else state = game_state::Dungeon;
    return;
  }

  if (state == game_state::Dungeon)
  {
    int dx = 0, dy = 0;
    bool acted = false;
    if (ch == 'q') running = false;
    else if (ch == KEY_UP)
    {
      dy = -1;
      acted = true;
    }
    else if (ch == KEY_DOWN)
    {
      dy = +1;
      acted = true;
    }
    else if (ch == KEY_LEFT)
    {
      dx = -1;
      acted = true;
    }
    else if (ch == KEY_RIGHT)
    {
      dx = +1;
      acted = true;
    }
    else if (ch == 'f' || ch == 'F')
    {
      Systems::cast_fireball(reg, map, last_dx, last_dy, log, renderer);
      acted = true;
    }

    if (acted)
    {
      if (dx != 0 || dy != 0)
      {
        last_dx = dx;
        last_dy = dy;
      }
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
          inventory.push_back(reg.items[target]);
          log.add("Picked up " + reg.items[target].name);
          reg.destroy_entity(target);
        }
      }
      Systems::move_monsters(reg, map, log, scripts.lua);
      map.update_fov(p.x, p.y, reg.stats[reg.player_id].fov_range);
    }
  }
  else if (state == game_state::Inventory) { handle_input_inventory(ch); }
  // game_state::Help consumes input (ESC check above covers exit), so nothing else needed here.
}

void Game::render()
{
  renderer.clear_screen();

  if (state == game_state::Dungeon)
  {
    sol::table config = scripts.lua["get_level_config"](depth);
    int wc = config["wall_color"].get_or(4);
    int fc = config["floor_color"].get_or(1);
    std::string lvl_name = config["name"].get_or<std::string>("Dungeon");
    renderer.draw_dungeon(map, reg, log, reg.player_id, depth, wc, fc, lvl_name);
  }
  else if (state == game_state::Inventory) { renderer.draw_inventory(inventory, log); }
  else if (state == game_state::Stats) { renderer.draw_stats(reg, reg.player_id, reg.player_name, log); }
  else if (state == game_state::Help)
  {
    // Fetch help text from Lua
    std::string txt = scripts.lua["help_text"].get_or<std::string>("No help text defined in game.lua");
    renderer.draw_help(log, txt);
  }

  renderer.refresh_screen();
}

void Game::handle_input_inventory(int ch)
{
  if (ch >= '1' && ch <= '9')
  {
    size_t idx = ch - '1';
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
          log.add("Script Error: " + std::string(err.what()), ColorPair::Orc);
        }
      }
    }
  }
}

bool Game::handle_game_over()
{
  flushinp();
  renderer.draw_game_over(log);
  while (true)
  {
    int choice = getch();
    if (choice == 'r' || choice == 'R') return true;
    if (choice == 'q' || choice == 'Q') return false;
  }
}

bool Game::handle_victory()
{
  flushinp();
  renderer.draw_victory(log);
  while (true)
  {
    int choice = getch();
    if (choice == 'c' || choice == 'C') return true;
    if (choice == 'q' || choice == 'Q') return false;
  }
}
