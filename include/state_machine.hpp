//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#pragma once
#include "states/dungeon.hpp"
#include "states/game_over.hpp"
#include "states/help.hpp"
#include "states/inventory.hpp"
#include "states/setup.hpp"
#include "states/stats.hpp"
#include "states/tick.hpp"
#include "states/victory.hpp"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <variant>

namespace roguey
{
  class game;

  class state_machine
  {
  public:
    using state_variant = std::variant<setup_state,
                                       dungeon_state,
                                       inventory_state,
                                       stats_state,
                                       help_state,
                                       tick_state,
                                       game_over_state,
                                       victory_state>;

    state_machine(); // Initialize with default state (setup)

    ftxui::Element render(game& g);
    bool on_event(game& g, ftxui::Event event);

    template<typename T> void transition_to(T&& new_state) { current_state = std::forward<T>(new_state); }

  private:
    state_variant current_state;
  };
}
