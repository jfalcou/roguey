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

struct Renderable
{
  char glyph;
  std::string color; // Changed from int to string key
};

class Registry
{
public:
  std::map<EntityID, Position> positions;
  std::map<EntityID, Renderable> renderables;
  std::map<EntityID, Stats> stats;
  std::map<EntityID, ItemTag> items;

  std::map<EntityID, std::string> names;
  std::map<EntityID, std::string> script_paths;
  std::map<EntityID, std::string> monster_types;

  std::vector<EntityID> monsters;
  EntityID player_id = 0;
  EntityID boss_id = 0;
  std::size_t next_id = 1;
  std::string player_name = "Hero";
  std::string player_class_script;

  EntityID create_entity() { return next_id++; }

  void destroy_entity(EntityID id);
};
