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
#include <string>

namespace roguey
{
  class game;

  struct setup_state
  {
    ftxui::Element render(game& g);
    bool on_event(game& g, ftxui::Event event);

    int step = 0;
    std::string input_buffer;
    int class_selection = 0;
  };
}
