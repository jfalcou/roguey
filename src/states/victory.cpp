//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "game.hpp"
#include "states/dungeon.hpp"
#include "states/victory.hpp"

namespace roguey
{
  ftxui::Element victory_state::render(game& g)
  {
    return g.renderer.render_victory(g.log);
  }

  bool victory_state::on_event(game& g, ftxui::Event event)
  {
    if (event == ftxui::Event::Character('c') || event == ftxui::Event::Character('C'))
    {
      g.scripts.load_script(g.current_level_script);
      g.reset(false, g.scripts.lua["get_next_level"](g.depth));
      g.set_state(dungeon_state{});
    }
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q')) { g.stop(); }
    return true;
  }
}
