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

  script_engine::script_engine(std::string const& main_script)
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
    lua.new_usertype<message_log>(
      "Log", "add",
      sol::overload([](message_log& l, std::string const& msg) { l.add(msg, "ui_default"); }, &message_log::add));

    sol::protected_function start_cfg_func = lua["get_start_config"];
    configuration = start_cfg_func();

    is_valid = true;
  }

  void script_engine::init_lua()
  {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

    // Register basic types
    lua.new_usertype<position>("Position", "x", &position::x, "y", &position::y);
    lua.new_usertype<stats>("Stats", "archetype", &stats::archetype, "hp", &stats::hp, "max_hp", &stats::max_hp, "mana",
                            &stats::mana, "max_mana", &stats::max_mana, "damage", &stats::damage, "xp", &stats::xp,
                            "level", &stats::level, "gold", &stats::gold, "fov", &stats::fov_range);
  }

  void script_engine::discover_assets()
  {
    class_templates.clear();

    if (fs::exists("scripts/class"))
    {
      for (auto const& entry : fs::directory_iterator("scripts/class"))
        if (entry.path().extension() == ".lua") class_templates.push_back(entry.path().string());
    }
  }

  bool script_engine::load_script(std::string const& path)
  {
    auto res = lua.safe_script_file(systems::checked_script_path(path), sol::script_pass_on_error);
    return res.valid();
  }

  std::string script_engine::pick_from_weights(sol::table weights, std::mt19937& gen)
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
