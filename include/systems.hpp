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
#include <sol/sol.hpp>
#include <string>

namespace roguey
{
  class Renderer;

  struct MessageLog
  {
    struct Entry
    {
      std::string text;
      std::string color;
    };

    std::vector<Entry> messages;
    size_t max_messages = 50;
    void add(std::string msg, std::string const& color = "ui_default");
  };

  namespace Systems
  {
    std::string checked_script_path(std::string_view path);

    EntityID get_entity_at(Registry const& reg, int x, int y, EntityID ignore_id = 0);
    void attack(Registry& reg, EntityID a_id, EntityID d_id, MessageLog& log, sol::state& lua);
    void check_level_up(Registry& reg, MessageLog& log, sol::state& lua);
    void cast_fireball(Registry& reg, Dungeon& map, int dx, int dy, MessageLog& log, sol::state& lua);

    void update_projectiles(Registry& reg, Dungeon const& map, MessageLog& log, sol::state& lua);

    void move_monsters(Registry& reg, Dungeon const& map, MessageLog& log, sol::state& lua);
  }
}
