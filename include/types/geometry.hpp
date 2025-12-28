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
  struct Position
  {
    int x, y;

    bool operator<(Position const& other) const { return y < other.y || (y == other.y && x < other.x); }

    bool operator==(Position const& other) const { return x == other.x && y == other.y; }
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
}
