//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================

#include "game.hpp"
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
  bool debug = false;
  std::vector<std::string> args(argv + 1, argv + argc);
  for (auto const& arg : args)
  {
    if (arg == "-d") debug = true;
  }

  Systems::set_binary_path(argv[0]);
  Game game(debug);
  game.run();
  return 0;
}
