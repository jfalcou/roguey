//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include <cassert>
#include <cctype>
#include <random>
#include <string>

namespace roguey
{
  // Parse a string containing a chain of dice spec and modifier
  // ie: D8, 1D6+2, 3D4-3, -5d6 + 2D10
  // Then roll said dices as it goes
  template<typename PRNG> int roll(std::string const& s, PRNG& gen)
  {
    assert(!s.empty());

    int grand_total = 0;
    size_t i = 0;
    size_t len = s.length();
    int term_sign = 1;
    bool is_first = true;

    auto skip_spaces = [&]() {
      while (i < len && std::isspace(s[i])) { i++; }
    };

    while (i < len)
    {
      skip_spaces();
      if (i >= len) break;

      if (s[i] == '+' || s[i] == '-')
      {
        term_sign = (s[i] == '-') ? -1 : 1;
        i++;
        skip_spaces();
      }
      else if (!is_first) { assert(false); }

      int num = 0;
      bool has_digits = false;

      while (i < len && std::isdigit(s[i]))
      {
        num = num * 10 + (s[i] - '0');
        has_digits = true;
        i++;
      }

      skip_spaces();

      if (i < len && (s[i] == 'd' || s[i] == 'D'))
      {
        int n_dice = has_digits ? num : 1;

        i++;
        skip_spaces();

        int faces = 0;
        bool has_faces = false;
        while (i < len && std::isdigit(s[i]))
        {
          faces = faces * 10 + (s[i] - '0');
          has_faces = true;
          i++;
        }
        assert(has_faces && faces > 0);

        std::uniform_int_distribution<> distrib(1, faces);
        int sub_total = 0;

        for (int k = 0; k < n_dice; ++k) { sub_total += distrib(gen); }

        grand_total += (term_sign * sub_total);
      }
      else
      {
        assert(has_digits);
        grand_total += (term_sign * num);
      }

      is_first = false;
    }

    return grand_total;
  }
}
