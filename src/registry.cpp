//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================

#include "registry.hpp"
#include <algorithm>

void Registry::destroy_entity(EntityID id)
{
  positions.erase(id);
  renderables.erase(id);
  items.erase(id);
  stats.erase(id);
  script_paths.erase(id);
  monster_types.erase(id);
  names.erase(id);

  auto it = std::find(monsters.begin(), monsters.end(), id);
  if (it != monsters.end()) monsters.erase(it);
  if (id == boss_id) boss_id = 0;
}
