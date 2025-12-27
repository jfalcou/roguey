//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================

#include "dungeon.hpp"
#include <algorithm>
#include <cmath>
#include <random>

Dungeon::Dungeon(int w, int h) : width(w), height(h)
{
  grid.assign(height, std::vector<char>(width, '#'));
  explored.assign(height, std::vector<bool>(width, false));
}

void Dungeon::generate()
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis_w(6, 12), dis_h(4, 7), dis_x(1, width - 13), dis_y(1, height - 8);
  rooms.clear();
  grid.assign(height, std::vector<char>(width, '#'));
  explored.assign(height, std::vector<bool>(width, false));

  for (int i = 0; i < 50; ++i)
  {
    Rect room{dis_x(gen), dis_y(gen), dis_w(gen), dis_h(gen)};
    if (std::none_of(rooms.begin(), rooms.end(), [&](Rect const& r) { return room.intersects(r); }))
    {
      for (int y = room.y; y < room.y + room.h; ++y)
        for (int x = room.x; x < room.x + room.w; ++x) grid[y][x] = '.';
      if (!rooms.empty())
      {
        Position p1 = rooms.back().center(), p2 = room.center();
        if (gen() % 2)
        {
          carve_h(p1.x, p2.x, p1.y);
          carve_v(p1.y, p2.y, p2.x);
        }
        else
        {
          carve_v(p1.y, p2.y, p1.x);
          carve_h(p1.x, p2.x, p2.y);
        }
      }
      rooms.push_back(room);
    }
  }
}

void Dungeon::carve_h(int x1, int x2, int y)
{
  for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) grid[y][x] = '.';
}

void Dungeon::carve_v(int y1, int y2, int x)
{
  for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) grid[y][x] = '.';
}

bool Dungeon::is_walkable(int x, int y) const
{
  return y >= 0 && y < height && x >= 0 && x < width && grid[y][x] == '.';
}

void Dungeon::update_fov(int px, int py, int range)
{
  visible_tiles.clear();
  for (int i = 0; i < 360; i += 2)
  {
    double rad = i * 0.0174533;
    double ox = std::cos(rad), oy = std::sin(rad);
    double cur_x = px + 0.5, cur_y = py + 0.5;
    for (int r = 0; r < range; r++)
    {
      int ix = (int)cur_x, iy = (int)cur_y;
      if (ix < 0 || ix >= width || iy < 0 || iy >= height) break;
      visible_tiles.insert({ix, iy});
      explored[iy][ix] = true;
      if (grid[iy][ix] == '#') break;
      cur_x += ox;
      cur_y += oy;
    }
  }
}
