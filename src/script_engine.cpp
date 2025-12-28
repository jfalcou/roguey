//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "script_engine.hpp"
#include "systems.hpp"
#include <filesystem>

namespace roguey
{
  namespace fs = std::filesystem;

  ScriptEngine::ScriptEngine(std::string const& main_script)
  {
    init_lua();
    if (!load_script(main_script))
    {
      is_valid = false;
      return;
    }

    // Retrieve list of global options
    discover_assets();

    // Global log in LUA
    lua.new_usertype<MessageLog>(
      "Log", "add",
      sol::overload([](MessageLog& l, std::string const& msg) { l.add(msg, "ui_default"); }, &MessageLog::add));

    sol::protected_function start_cfg_func = lua["get_start_config"];
    configuration = start_cfg_func();

    is_valid = true;
  }

  void ScriptEngine::init_lua()
  {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

    // Register basic types
    lua.new_usertype<Position>("Position", "x", &Position::x, "y", &Position::y);
    lua.new_usertype<Stats>("Stats", "archetype", &Stats::archetype, "hp", &Stats::hp, "max_hp", &Stats::max_hp, "mana",
                            &Stats::mana, "max_mana", &Stats::max_mana, "damage", &Stats::damage, "xp", &Stats::xp,
                            "level", &Stats::level, "gold", &Stats::gold, "fov", &Stats::fov_range);
  }

  void ScriptEngine::discover_assets()
  {
    class_templates.clear();

    if (fs::exists("scripts/class"))
    {
      for (auto const& entry : fs::directory_iterator("scripts/class"))
        if (entry.path().extension() == ".lua") class_templates.push_back(entry.path().string());
    }
  }

  bool ScriptEngine::load_script(std::string const& path)
  {
    auto res = lua.safe_script_file(Systems::checked_script_path(path), sol::script_pass_on_error);
    return res.valid();
  }

  std::string ScriptEngine::pick_from_weights(sol::table weights, std::mt19937& gen)
  {
    int total_weight = 0;
    std::vector<std::pair<std::string, int>> pool;
    for (auto const& [key, val] : weights)
    {
      int w = val.as<int>();
      total_weight += w;
      pool.push_back({key.as<std::string>(), total_weight});
    }
    if (total_weight <= 0) return "";
    int roll = std::uniform_int_distribution<>(1, total_weight)(gen);
    for (auto const& p : pool)
    {
      if (roll <= p.second) return p.first;
    }
    return "";
  }
}
