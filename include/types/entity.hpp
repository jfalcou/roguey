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

namespace roguey
{
  using entity_id = std::uint64_t;

  struct stats
  {
    std::string archetype;
    int hp, max_hp, mana, max_mana, damage, xp, level, fov_range, gold;
    int action_delay, action_timer;
  };
}
