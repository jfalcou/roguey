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
  class renderer;

  struct message_log
  {
    struct entry
    {
      std::string text;
      std::string color;
    };

    std::vector<entry> messages;
    size_t max_messages = 50;
    void add(std::string msg, std::string const& color = "ui_default");
  };

  namespace systems
  {

    struct entity_data
    {
      stats stats;
      renderable render;
      std::string name;
      std::string type;
    };

    std::optional<sol::table> try_get_table(sol::state& lua, std::string const& name, message_log& log);
    entity_data parse_entity_config(sol::table const& t, std::string_view default_name = "Unknown");

    std::string checked_script_path(std::string_view path);

    entity_id get_entity_at(registry const& reg, int x, int y, entity_id ignore_id = 0);
    void attack(registry& reg, entity_id a_id, entity_id d_id, message_log& log, sol::state& lua);
    void check_level_up(registry& reg, message_log& log, sol::state& lua);
    void cast_fireball(registry& reg, dungeon& map, int dx, int dy, message_log& log, sol::state& lua);

    // Returns true if a visual change occurred (movement, damage, death)
    bool update_projectiles(registry& reg, dungeon const& map, message_log& log, sol::state& lua);

    // Returns true if a visual change occurred
    bool move_monsters(registry& reg, dungeon const& map, message_log& log, sol::state& lua);

    bool execute_script(sol::state& lua, std::string const& path, message_log& log);
  }
}
