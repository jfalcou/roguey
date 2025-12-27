#pragma once
#include "registry.hpp" // For Stats/Position if needed by Lua registration
#include "sol/sol.hpp"
#include "types.hpp"
#include <filesystem>
#include <random>
#include <string>
#include <vector>

class ScriptEngine
{
public:
  sol::state lua;
  std::vector<std::string> monster_templates;
  std::vector<std::string> class_templates;
  std::vector<std::string> item_templates;

  ScriptEngine();
  void init_lua();
  void discover_assets();
  bool load_script(std::string const& path);
  std::string pick_from_weights(sol::table weights, std::mt19937& gen);
};
