//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include <vector>

namespace roguey
{
  template<typename Element> struct grid2D : private std::vector<Element>
  {
  public:
    using data_type = std::vector<Element>;

    grid2D(int width, int height, Element value = {}) : data_type(width * height, value), width_(width), height_(height)
    {
    }

    using data_type::begin;
    using data_type::empty;
    using data_type::end;
    using data_type::size;

    decltype(auto) operator()(int x, int y) const { return data_type::operator[](x + y * width_); }

    decltype(auto) operator()(int x, int y) { return data_type::operator[](x + y * width_); }

  private:
    int width_, height_;
  };
}
