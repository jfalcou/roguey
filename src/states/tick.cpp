//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "game.hpp"
#include "states/dungeon.hpp"
#include "states/tick.hpp"

namespace roguey
{
  ftxui::Element tick_state::render(game& g)
  {
    sol::table config = g.scripts.lua["get_level_config"](g.depth);
    std::string level_name = config["name"].get_or<std::string>("Unknown");
    std::string wall_color = config["wall_color"].get_or<std::string>("asset_wall");
    std::string floor_color = config["floor_color"].get_or<std::string>("asset_floor");
    return g.renderer.render_dungeon(g.map, g.reg, g.log, g.reg.player_id, g.depth, level_name, wall_color,
                                     floor_color);
  }

  bool tick_state::on_event(game& g, ftxui::Event event)
  {
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q'))
    {
      g.stop();
      return true;
    }

    if (event == ftxui::Event::Special({0}))
    {
      if (g.menu_lock > 0) g.menu_lock--;

      if (g.reg.stats.contains(g.reg.player_id))
      {
        auto& s = g.reg.stats[g.reg.player_id];
        if (s.action_timer > 0) s.action_timer--;
      }

      // Execute systems (ignoring return values to be safe)
      systems::update_projectiles(g.reg, g.map, g.log, g.scripts.lua);
      systems::move_monsters(g.reg, g.map, g.log, g.scripts.lua);

      if (g.reg.stats[g.reg.player_id].action_timer == 0) { g.set_state(dungeon_state{}); }

      // Always return true on tick to ensure animation frames are drawn
      return true;
    }
    else if (event.is_character() || event == ftxui::Event::ArrowUp || event == ftxui::Event::ArrowDown ||
             event == ftxui::Event::ArrowLeft || event == ftxui::Event::ArrowRight)
    {
      g.buffered_event = event;
      g.has_buffered_event = true;
    }

    return true;
  }
}
