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
  enum class item_type
  {
    Gold,
    Consumable,
    Stairs
  };

  struct item_tag
  {
    item_type type;
    int value;
    std::string name;
    std::string script;
  };
}
