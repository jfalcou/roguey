//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once

namespace roguey
{
  struct position
  {
    int x, y;

    bool operator<(position const& other) const { return y < other.y || (y == other.y && x < other.x); }

    bool operator==(position const& other) const { return x == other.x && y == other.y; }
  };

  struct rectangle
  {
    int x, y, w, h;

    position center() const { return {x + w / 2, y + h / 2}; }

    bool intersects(rectangle const& other) const
    {
      return x <= other.x + other.w && x + w >= other.x && y <= other.y + other.h && y + h >= other.y;
    }
  };
}
