//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "game.hpp"
#include "states/dungeon.hpp"
#include "states/setup.hpp"

namespace roguey
{
  ftxui::Element setup_state::render(game& g)
  {
    if (step == 0) return g.renderer.render_character_creation(input_buffer, g.log);
    if (step == 1) return g.renderer.render_class_selection(g.scripts.class_templates, class_selection, g.log);
    return ftxui::text("Unknown Setup Step");
  }

  bool setup_state::on_event(game& g, ftxui::Event event)
  {
    if (event == ftxui::Event::Special({0})) return false; // Ignore ticks, no animation here

    if (event == ftxui::Event::Special({3}))
    {
      g.stop();
      return true;
    }

    if (step == 0)
    {
      if (event.is_character()) { input_buffer += event.character(); }
      else if (event == ftxui::Event::Backspace)
      {
        if (!input_buffer.empty()) input_buffer.pop_back();
      }
      else if (event == ftxui::Event::Return)
      {
        if (!input_buffer.empty())
        {
          g.reg.player_name = input_buffer;
          g.log.add("Welcome, " + g.reg.player_name + "!", "ui_text");
          step = 1;
        }
      }
    }
    else if (step == 1)
    {
      if (event == ftxui::Event::ArrowUp && class_selection > 0) class_selection--;
      if (event == ftxui::Event::ArrowDown && class_selection < (int)g.scripts.class_templates.size() - 1)
        class_selection++;
      if (event == ftxui::Event::Return || event == ftxui::Event::Character(' '))
      {
        g.reg.player_class_script = g.scripts.class_templates[class_selection];

        std::string first_level =
          g.scripts.configuration["start_level"].get_or<std::string>("scripts/levels/dungeon.lua");
        std::string init_msg = g.scripts.configuration["initial_log_message"].get_or<std::string>("Welcome!");

        g.log.add(init_msg);
        g.reset(true, first_level);
        g.set_state(dungeon_state{});
      }
    }
    return true;
  }
}
