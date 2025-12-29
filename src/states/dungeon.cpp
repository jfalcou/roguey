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
#include "states/help.hpp"
#include "states/inventory.hpp"
#include "states/stats.hpp"
#include "states/tick.hpp"
#include "states/victory.hpp"

namespace roguey
{
  bool is_movement(ftxui::Event e)
  {
    return e == ftxui::Event::ArrowUp || e == ftxui::Event::ArrowDown || e == ftxui::Event::ArrowLeft ||
           e == ftxui::Event::ArrowRight;
  }

  ftxui::Element dungeon_state::render(game& g)
  {
    if (g.reg.stats.contains(g.reg.player_id) && g.reg.stats[g.reg.player_id].hp <= 0)
    {
      return g.renderer.render_game_over(g.log);
    }
    if (g.reg.boss_id != 0 && !g.reg.positions.contains(g.reg.boss_id)) { return g.renderer.render_victory(g.log); }

    sol::table config = g.scripts.lua["get_level_config"](g.depth);
    std::string level_name = config["name"].get_or<std::string>("Unknown");
    std::string wall_color = config["wall_color"].get_or<std::string>("asset_wall");
    std::string floor_color = config["floor_color"].get_or<std::string>("asset_floor");

    return g.renderer.render_dungeon(g.map, g.reg, g.log, g.reg.player_id, g.depth, level_name, wall_color,
                                     floor_color);
  }

  bool dungeon_state::on_event(game& g, ftxui::Event event)
  {
    ftxui::Event active_event = event;

    if (g.has_buffered_event && g.reg.stats[g.reg.player_id].action_timer == 0)
    {
      if (event.is_character() || is_movement(event))
      {
        ftxui::Event next_buffer = event;
        active_event = g.buffered_event;
        g.buffered_event = next_buffer;
        g.has_buffered_event = true;
      }
      else
      {
        active_event = g.buffered_event;
        g.has_buffered_event = false;
      }
    }

    if (!g.reg.stats.contains(g.reg.player_id)) return false;
    if (g.reg.stats[g.reg.player_id].hp <= 0)
    {
      g.set_state(game_over_state{});
      return true;
    }
    if (g.reg.boss_id != 0 && !g.reg.positions.contains(g.reg.boss_id))
    {
      g.set_state(victory_state{});
      return true;
    }

    // Optimization: Only redraw on tick IF projectiles moved
    if (active_event == ftxui::Event::Special({0}))
    {
      return systems::update_projectiles(g.reg, g.map, g.log, g.scripts.lua);
    }

    if (active_event == ftxui::Event::Character('q') || active_event == ftxui::Event::Character('Q'))
    {
      g.stop();
      return true;
    }

    if (active_event == ftxui::Event::Character('i'))
    {
      g.set_menu_lock(5);
      g.set_state(inventory_state{});
      return true;
    }
    if (active_event == ftxui::Event::Character('c'))
    {
      g.set_menu_lock(5);
      g.set_state(stats_state{});
      return true;
    }
    if (active_event == ftxui::Event::Escape)
    {
      g.set_state(help_state{});
      return true;
    }

    if (g.reg.stats[g.reg.player_id].action_timer > 0)
    {
      if (active_event.is_character() || is_movement(active_event))
      {
        g.buffered_event = active_event;
        g.has_buffered_event = true;
      }
      return true;
    }

    int dx = 0, dy = 0;
    bool acted = false;

    if (active_event == ftxui::Event::ArrowUp)
    {
      dy = -1;
      acted = true;
    }
    else if (active_event == ftxui::Event::ArrowDown)
    {
      dy = +1;
      acted = true;
    }
    else if (active_event == ftxui::Event::ArrowLeft)
    {
      dx = -1;
      acted = true;
    }
    else if (active_event == ftxui::Event::ArrowRight)
    {
      dx = +1;
      acted = true;
    }
    else if (active_event == ftxui::Event::Character('f'))
    {
      systems::cast_fireball(g.reg, g.map, g.last_dx, g.last_dy, g.log, g.scripts.lua);
      acted = true;
    }

    if (acted)
    {
      auto& s = g.reg.stats[g.reg.player_id];
      s.action_timer = s.action_delay;

      if (dx != 0 || dy != 0)
      {
        g.last_dx = dx;
        g.last_dy = dy;

        position& p = g.reg.positions[g.reg.player_id];
        entity_id target = systems::get_entity_at(g.reg, p.x + dx, p.y + dy);

        if (target && g.reg.stats.contains(target))
        {
          systems::attack(g.reg, g.reg.player_id, target, g.log, g.scripts.lua);
        }
        else if (g.map.is_walkable(p.x + dx, p.y + dy))
        {
          p.x += dx;
          p.y += dy;
          if (target && g.reg.items.contains(target))
          {
            if (g.reg.items[target].type == item_type::Stairs)
            {
              std::string next_lvl = g.reg.items[target].name;
              g.reset(false, next_lvl);
              g.set_state(dungeon_state{});
              return true;
            }

            if (systems::execute_script(g.scripts.lua, g.reg.items[target].script, g.log))
            {
              if (g.scripts.lua["on_pick"](g.reg.stats[g.reg.player_id], g.log))
              {
                g.inventory.push_back(g.reg.items[target]);
              }
            }
            g.reg.destroy_entity(target);
          }
        }
      }

      g.map.update_fov(g.reg.positions[g.reg.player_id].x, g.reg.positions[g.reg.player_id].y,
                       g.reg.stats[g.reg.player_id].fov_range);

      g.set_state(tick_state{});
    }

    return true;
  }
}
