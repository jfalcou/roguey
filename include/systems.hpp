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
#include <vector>

namespace roguey
{
  class Renderer;

  struct LogEntry
  {
    std::string text;
    std::string color;
  };

  class MessageLog
  {
  public:
    std::vector<LogEntry> messages;
    std::size_t const max_messages = 10;

    void add(std::string msg, std::string const& color = "ui_default");
  };

  namespace Systems
  {
    EntityID get_entity_at(Registry const& reg, int x, int y, EntityID ignore_id = 0);

    void check_level_up(Registry& reg, MessageLog& log, sol::state& lua);
    void attack(Registry& reg, EntityID a_id, EntityID d_id, MessageLog& log, sol::state& lua);

    void cast_fireball(Registry& reg, Dungeon& map, int dx, int dy, MessageLog& log, sol::state& lua);
    void move_monsters(Registry& reg, Dungeon const& map, MessageLog& log, sol::state& lua);

    std::string checked_script_path(std::string_view path);
    void update_projectiles(
      Registry& reg, Dungeon const& map, MessageLog& log, sol::state& lua, Renderer* renderer = nullptr);
    std::string binary_path();
    void set_binary_path(std::string_view new_path);
  }
}
