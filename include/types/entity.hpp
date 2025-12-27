//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include <cstdint>

using EntityID = std::uint32_t;

struct Stats
{
  int hp, max_hp, mana, max_mana, damage, xp, level, fov_range;
};
