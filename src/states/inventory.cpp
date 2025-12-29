//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "game.hpp"
#include "states/dungeon.hpp"
#include "states/inventory.hpp"
#include "states/tick.hpp"

namespace roguey
{
  ftxui::Element inventory_state::render(game& g)
  {
    return g.renderer.render_inventory(g.inventory, g.log);
  }

  bool inventory_state::on_event(game& g, ftxui::Event event)
  {
    if (g.menu_lock > 0) return true;

    if (event == ftxui::Event::Escape || event == ftxui::Event::Character('i'))
    {
      g.set_state(dungeon_state{});
      return true;
    }

    if (event.is_character())
    {
      char ch = event.character()[0];
      if (ch >= '1' && ch <= '9')
      {
        std::size_t idx = ch - '1';
        if (idx < g.inventory.size())
        {
          auto& item = g.inventory[idx];
          if (systems::execute_script(g.scripts.lua, item.script, g.log))
          {
            sol::protected_function on_use = g.scripts.lua["on_use"];
            auto use_res = on_use(g.reg.stats[g.reg.player_id], g.log);

            if (use_res.valid())
            {
              // Item used successfully
              g.inventory.erase(g.inventory.begin() + idx);

              // Apply Turn Cost
              if (g.reg.stats.contains(g.reg.player_id))
              {
                auto& s = g.reg.stats[g.reg.player_id];
                s.action_timer = s.action_delay; // Using item takes one turn
              }

              // Transition to Animating State to process the turn
              g.set_state(tick_state{});
            }
            else
            {
              sol::error err = use_res;
              g.log.add("Script Error: " + std::string(err.what()), "ui_emphasis");
            }
          }
        }
      }
    }
    return true;
  }
}
