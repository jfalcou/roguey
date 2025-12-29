//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include "registry.hpp"
#include <random>
#include <sol/sol.hpp>
#include <string>
#include <vector>

namespace roguey
{
  class script_engine
  {
  public:
    sol::state lua;
    sol::table configuration;
    bool is_valid;
    std::vector<std::string> class_templates;

    script_engine(std::string const& main_script);

    bool load_script(std::string const& path);
    std::string pick_from_weights(sol::table weights, std::mt19937& gen);

  private:
    void init_lua();
    void discover_assets();
  };
}
