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
  int color;
};

struct Registry
{
  std::map<EntityID, Position> positions;
  std::map<EntityID, Renderable> renderables;
  std::map<EntityID, ItemTag> items;
  std::map<EntityID, Stats> stats;
  std::map<EntityID, std::string> script_paths;
  std::map<EntityID, std::string> monster_types;
  std::map<EntityID, std::string> names;
  std::vector<EntityID> monsters;

  EntityID player_id;
  EntityID boss_id = 0;
  std::string player_name = "Hero";
  std::string player_class_script = "";
  int player_gold = 0;
  EntityID next_id = 1;

  EntityID create_entity() { return next_id++; }

  void destroy_entity(EntityID id);
};
