//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "game.hpp"
#include "states/dungeon.hpp"
#include "states/game_over.hpp"

namespace roguey
{
  ftxui::Element game_over_state::render(game& g)
  {
    return g.renderer.render_game_over(g.log);
  }

  bool game_over_state::on_event(game& g, ftxui::Event event)
  {
    if (event == ftxui::Event::Character('r') || event == ftxui::Event::Character('R'))
    {
      sol::table config = g.scripts.lua["get_start_config"]();
      std::string start_level = config["start_level"];
      g.log.add(config["initial_log_message"]);
      g.reset(true, start_level);
      g.set_state(dungeon_state{});
    }
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q')) { g.stop(); }
    return true;
  }
}
