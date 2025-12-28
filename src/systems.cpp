//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================
#include "renderer.hpp"
#include "systems.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace roguey
{
  namespace Systems
  {
    std::string checked_script_path(std::string_view path)
    {
      // we expect script files to be located in a directory relative to the current executable location
      namespace fs = std::filesystem;
      auto complete_path = fs::canonical(path);
      assert(fs::exists(complete_path));
      return complete_path.string();
    }
  }

  void MessageLog::add(std::string msg, std::string const& color)
  {
    messages.push_back({msg, color});
    if (messages.size() > max_messages) messages.erase(messages.begin());
  }

  EntityID Systems::get_entity_at(Registry const& reg, int x, int y, EntityID ignore_id)
  {
    EntityID found = 0;
    for (auto const& [id, pos] : reg.positions)
    {
      if (id == ignore_id) continue; // Skip self

      if (pos.x == x && pos.y == y)
      {
        // Priority: Creatures (Stats) > Items/Others
        if (reg.stats.contains(id)) return id;
        found = id;
      }
    }
    return found;
  }

  void Systems::attack(Registry& reg, EntityID a_id, EntityID d_id, MessageLog& log, sol::state& lua)
  {
    auto& a = reg.stats[a_id];
    auto& d = reg.stats[d_id];
    d.hp -= a.damage;

    // Get names or fallbacks
    std::string a_name = reg.names.count(a_id) ? reg.names.at(a_id) : "Unknown";
    std::string d_name = reg.names.count(d_id) ? reg.names.at(d_id) : "Unknown";

    if (a_id == reg.player_id) { log.add("You hit the " + d_name + " for " + std::to_string(a.damage), "ui_default"); }
    else if (d_id == reg.player_id)
    {
      log.add("The " + a_name + " hits you for " + std::to_string(a.damage), "ui_emphasis");
    }

    if (d.hp <= 0)
    {
      if (a_id == reg.player_id)
      {
        log.add("You defeated the " + d_name + "! +50 XP", "ui_gold");
        reg.stats[a_id].xp += 50;
        check_level_up(reg, log, lua);
      }
      else if (d_id == reg.player_id) { log.add(d_name + " was defeated by the " + a_name, "ui_emphasis"); }
      reg.destroy_entity(d_id);
    }
  }

  void Systems::check_level_up(Registry& reg, MessageLog& log, sol::state& lua)
  {
    auto& s = reg.stats[reg.player_id];
    int next_lvl_xp = s.level * 100;

    if (s.xp >= next_lvl_xp)
    {
      s.level++;
      auto res = lua.safe_script_file(Systems::checked_script_path(reg.player_class_script), sol::script_pass_on_error);
      if (!res.valid()) return;

      sol::table current_stats = lua.create_table();
      current_stats["hp"] = s.max_hp;
      current_stats["mp"] = s.max_mana;
      current_stats["damage"] = s.damage;

      sol::protected_function level_func = lua["level_up"];
      auto level_res = level_func(current_stats);

      if (level_res.valid())
      {
        sol::table new_stats = level_res;
        s.max_hp = new_stats["hp"];
        s.hp = s.max_hp;
        s.max_mana = new_stats["mp"];
        s.mana = s.max_mana;
        s.damage = new_stats["damage"];
        log.add("Level Up! You are now Level " + std::to_string(s.level), "ui_gold");
      }
    }
  }

  void Systems::cast_fireball(Registry& reg, Dungeon& map, int dx, int dy, MessageLog& log, sol::state& lua)
  {
    std::string script_path = "scripts/spells/fireball.lua";

    // 1. Load Script
    auto result = lua.safe_script_file(script_path, sol::script_pass_on_error);
    if (!result.valid())
    {
      sol::error err = result;
      log.add("Spell Error: " + std::string(err.what()), "ui_failure");
      return;
    }

    // 2. Read Config Table
    sol::table data = lua["spell_data"];
    if (!data.valid())
    {
      log.add("Spell Error: Missing spell_data", "ui_failure");
      return;
    }

    int mana_cost = data.get_or("mana_cost", 10);
    int damage = data.get_or("damage", 10);
    int range = data.get_or("range", 10);
    std::string glyph_str = data.get_or<std::string>("glyph", "*");
    char glyph = glyph_str.empty() ? '*' : glyph_str[0];
    std::string color = data.get_or<std::string>("color", "ui_default");
    std::string name = data.get_or<std::string>("name", "Spell");

    // 3. Check Mana
    auto& s = reg.stats[reg.player_id];
    if (s.mana < mana_cost)
    {
      log.add("Not enough mana! (" + std::to_string(mana_cost) + " required)", "ui_emphasis");
      return;
    }
    s.mana -= mana_cost;

    Position p = reg.positions[reg.player_id];
    int start_x = p.x + dx;
    int start_y = p.y + dy;

    if (!map.is_walkable(start_x, start_y))
    {
      log.add("You cast a " + name + " into the wall...", "ui_default");
      return;
    }

    EntityID id = reg.create_entity();
    reg.positions[id] = p; // Start at player pos
    reg.renderables[id] = {glyph, color};
    reg.projectiles[id] = {dx, dy, damage, range, reg.player_id};
    reg.script_paths[id] = script_path;
    reg.names[id] = name;

    log.add("You cast a " + name + "!", color);
  }

  void Systems::update_projectiles(
    Registry& reg, Dungeon const& map, MessageLog& log, sol::state& lua, Renderer* renderer)
  {
    std::vector<EntityID> to_destroy;

    for (auto& [id, proj] : reg.projectiles)
    {
      if (!reg.positions.contains(id))
      {
        to_destroy.push_back(id);
        continue;
      }

      if (proj.range <= 0)
      {
        log.add(reg.names[id] + " fizzles out.", "ui_default");
        to_destroy.push_back(id);
        continue;
      }
      proj.range--;

      auto& pos = reg.positions[id];

      if (reg.script_paths.contains(id))
      {
        auto script_res = lua.safe_script_file(reg.script_paths[id], sol::script_pass_on_error);
        if (script_res.valid())
        {
          sol::protected_function update_func = lua["update_projectile"];
          auto res = update_func(pos.x, pos.y, proj.dx, proj.dy);
          if (res.valid())
          {
            proj.dx = res[0];
            proj.dy = res[1];
          }
        }
      }

      int tx = pos.x + proj.dx;
      int ty = pos.y + proj.dy;

      if (renderer)
      {
        char glyph = '*';
        std::string color = "ui_default";
        if (reg.renderables.contains(id))
        {
          glyph = reg.renderables[id].glyph;
          color = reg.renderables[id].color;
        }

        char restore = map.grid(tx, ty);
        EntityID other = get_entity_at(reg, tx, ty, id);
        if (other != 0 && reg.renderables.contains(other)) { restore = reg.renderables[other].glyph; }

        renderer->animate_projectile(tx, ty, glyph, color, restore);
      }

      if (!map.is_walkable(tx, ty))
      {
        log.add(reg.names[id] + " hits a wall.", "ui_default");
        to_destroy.push_back(id);
        continue;
      }

      EntityID target = get_entity_at(reg, tx, ty, id);

      if (target != 0 && target != proj.owner)
      {
        if (reg.stats.contains(target))
        {
          reg.stats[target].hp -= proj.damage;
          std::string t_name = reg.names.count(target) ? reg.names.at(target) : "Target";
          log.add(reg.names[id] + " burns " + t_name + " for " + std::to_string(proj.damage), "fx_fire");

          if (reg.stats[target].hp <= 0)
          {
            log.add(t_name + " incinerated.", "ui_gold");
            reg.destroy_entity(target);
          }
        }
        to_destroy.push_back(id);
      }
      else
      {
        pos.x = tx;
        pos.y = ty;
      }
    }

    for (auto id : to_destroy) reg.destroy_entity(id);
  }

  void Systems::move_monsters(Registry& reg, Dungeon const& map, MessageLog& log, sol::state& lua)
  {
    if (!reg.positions.contains(reg.player_id)) return;
    Position p_pos = reg.positions.at(reg.player_id);

    for (auto m_id : reg.monsters)
    {
      if (!reg.positions.contains(m_id) || !reg.script_paths.contains(m_id)) continue;
      auto& m_pos = reg.positions[m_id];
      std::string const& script = reg.script_paths[m_id];

      auto script_res = lua.safe_script_file(Systems::checked_script_path(script), sol::script_pass_on_error);
      if (!script_res.valid()) continue;

      sol::protected_function ai_func = lua["update_ai"];
      auto res = ai_func(m_pos.x, m_pos.y, p_pos.x, p_pos.y);

      if (res.valid())
      {
        int dx = res[0], dy = res[1];
        if (dx != 0 || dy != 0)
        {
          int tx = m_pos.x + dx, ty = m_pos.y + dy;
          if (tx == p_pos.x && ty == p_pos.y) attack(reg, m_id, reg.player_id, log, lua);
          else if (map.is_walkable(tx, ty) && get_entity_at(reg, tx, ty) == 0)
          {
            m_pos.x = tx;
            m_pos.y = ty;
          }
        }
      }
    }
  }
}
