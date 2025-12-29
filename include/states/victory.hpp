//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace roguey
{
  class game;

  struct victory_state
  {
    ftxui::Element render(game& g);
    bool on_event(game& g, ftxui::Event event);
  };
}
