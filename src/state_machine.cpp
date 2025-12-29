//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "state_machine.hpp"
#include <variant>

namespace roguey
{
  state_machine::state_machine() : current_state(setup_state{}) {}

  ftxui::Element state_machine::render(game& g)
  {
    // The render call is visited on the current state
    return std::visit([&g](auto& s) { return s.render(g); }, current_state);
  }

  bool state_machine::on_event(game& g, ftxui::Event event)
  {
    // The event call is visited on the current state
    return std::visit([&g, event](auto& s) { return s.on_event(g, event); }, current_state);
  }
}
