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
  void Registry::destroy_entity(EntityID id)
  {
    positions.erase(id);
    renderables.erase(id);
    stats.erase(id);
    items.erase(id);
    names.erase(id);
    script_paths.erase(id);
    monster_types.erase(id);

    std::erase(monsters, id);
    if (id == boss_id) boss_id = 0;
  }
}
