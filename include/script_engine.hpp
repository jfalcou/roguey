//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include "registry.hpp"
#include "sol/sol.hpp"
#include "types.hpp"
#include <random>
#include <string>
#include <vector>

class ScriptEngine
{
public:
  sol::state lua;
  std::vector<std::string> class_templates;

  ScriptEngine();

  void init_lua();
  void discover_assets();
  bool load_script(std::string const& path);
  std::string pick_from_weights(sol::table weights, std::mt19937& gen);
};
