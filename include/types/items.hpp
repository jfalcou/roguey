//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include <string>

namespace roguey
{
  enum class ItemType
  {
    Gold,
    Consumable,
    Stairs
  };

  struct ItemTag
  {
    ItemType type;
    int value;
    std::string name;
    std::string script;
  };
}
