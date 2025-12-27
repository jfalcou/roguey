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

template<typename PRNG> int roll(std::string const& s, PRNG& gen)
{
  assert(!s.empty() && "Dice string cannot be empty");

  int n_dice = 0;
  int faces = 0;
  int modifier = 0;

  std::size_t i = 0;
  std::size_t len = s.length();

  auto skip_spaces = [&]() {
    while (i < len && std::isspace(s[i])) { i++; }
  };

  // Parse # of Dices
  skip_spaces();

  if (i < len && (s[i] == 'd' || s[i] == 'D')) { n_dice = 1; }
  else
  {
    while (i < len && std::isdigit(s[i]))
    {
      n_dice = n_dice * 10 + (s[i] - '0');
      i++;
    }
  }

  // --- Validate nDp+m
  skip_spaces();

  assert(i < len && (s[i] == 'd' || s[i] == 'D') && "Expected 'd' separator");
  i++;

  // Parse faces
  skip_spaces();

  bool found_faces = false;
  while (i < len && std::isdigit(s[i]))
  {
    faces = faces * 10 + (s[i] - '0');
    found_faces = true;
    i++;
  }

  assert(found_faces && faces > 0 && "Die must have a positive number of faces");

  // Parse Modifier
  skip_spaces();

  if (i < len)
  {
    assert((s[i] == '+' || s[i] == '-') && "Unexpected character; expected + or -");

    int sign = (s[i] == '-') ? -1 : 1;
    i++;

    skip_spaces();

    int val = 0;
    bool found_mod_digits = false;
    while (i < len && std::isdigit(s[i]))
    {
      val = val * 10 + (s[i] - '0');
      found_mod_digits = true;
      i++;
    }

    assert(found_mod_digits && "Expected digits after modifier sign");
    modifier = sign * val;
  }

  // Perform Roll
  std::uniform_int_distribution<int> distrib(1, faces);

  int total = 0;
  for (int k = 0; k < n_dice; ++k)
  {
    int r = distrib(gen);
    total += r;
  }
  total += modifier;

  return total;
}
