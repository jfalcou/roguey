//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include "types.hpp"
#include <random>
#include <set>
#include <vector>

namespace roguey
{
  class Dungeon
  {
  public:
    int width, height;
    grid2D<char> grid;
    grid2D<bool> explored;
    std::set<Position> visible_tiles;
    std::vector<Rect> rooms;

    Dungeon(int w, int h);
    void generate(std::mt19937& gen);
    void update_fov(int px, int py, int range);
    bool is_walkable(int x, int y) const;

  private:
    void carve_h(int x1, int x2, int y);
    void carve_v(int y1, int y2, int x);
  };
}
