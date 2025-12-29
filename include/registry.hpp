//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include "types.hpp"
#include <map>
#include <string>
#include <vector>

namespace roguey
{
  struct renderable
  {
    char glyph;
    std::string color;
  };

  struct projectile
  {
    int dx, dy;
    int damage;
    int range;
    entity_id owner;
    int action_delay = 2;
    int action_timer = 0;
  };

  class registry
  {
  public:
    std::map<entity_id, position> positions;
    std::map<entity_id, renderable> renderables;
    std::map<entity_id, stats> stats;
    std::map<entity_id, item_tag> items;
    std::map<entity_id, std::string> names;
    std::map<entity_id, std::string> script_paths;
    std::map<entity_id, std::string> monster_types;
    std::map<entity_id, projectile> projectiles;

    std::vector<entity_id> monsters;
    entity_id player_id = 0;
    entity_id boss_id = 0;
    entity_id next_id = 1;
    std::string player_name = "Hero";
    std::string player_class_script;

    entity_id create_entity() { return next_id++; }

    void clear();
    void destroy_entity(entity_id id);
  };
}
