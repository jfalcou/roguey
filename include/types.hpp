//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include <cstdint>
#include <string>

enum class game_state
{
  Dungeon,
  Inventory,
  Stats,
  Help,
  GameOver,
  Victory
};

enum class ItemType
{
  ui_gold,
  Consumable,
  Stairs
};

using EntityID = std::uint32_t;

struct Position
{
  int x, y;

  bool operator<(Position const& other) const { return y < other.y || (y == other.y && x < other.x); }

  bool operator==(Position const& other) const { return x == other.x && y == other.y; }
};

struct Stats
{
  int hp, max_hp, mana, max_mana, damage, xp, level, fov_range;
};

struct ItemTag
{
  ItemType type;
  int value;
  std::string name;
  std::string script;
};

struct Rect
{
  int x, y, w, h;

  Position center() const { return {x + w / 2, y + h / 2}; }

  bool intersects(Rect const& other) const
  {
    return x <= other.x + other.w && x + w >= other.x && y <= other.y + other.h && y + h >= other.y;
  }
};
