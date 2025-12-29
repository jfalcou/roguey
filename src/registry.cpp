//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================

#include "registry.hpp"
#include <algorithm>

namespace roguey
{
  void registry::destroy_entity(entity_id id)
  {
    positions.erase(id);
    renderables.erase(id);
    stats.erase(id);
    items.erase(id);
    names.erase(id);
    script_paths.erase(id);
    projectiles.erase(id);
    monster_types.erase(id);

    std::erase(monsters, id);
    if (id == boss_id) boss_id = 0;
  }

  void registry::clear()
  {
    positions.clear();
    renderables.clear();
    stats.clear();
    items.clear();
    projectiles.clear();
    names.clear();
    script_paths.clear();
    monster_types.clear();
    monsters.clear();

    boss_id = 0;
    next_id = 1;
  }
}
