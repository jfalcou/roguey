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
  namespace systems
  {
    std::string checked_script_path(std::string_view path)
    {
      // we expect script files to be located in a directory relative to the current executable location
      namespace fs = std::filesystem;
      auto complete_path = fs::canonical(path);
      assert(fs::exists(complete_path));
      return complete_path.string();
    }

    bool execute_script(sol::state& lua, std::string const& path, message_log& log)
    {
      auto res = lua.safe_script_file(checked_script_path(path), sol::script_pass_on_error);
      if (!res.valid())
      {
        sol::error err = res;
        namespace fs = std::filesystem;
        log.add("Script Error (" + fs::path(path).filename().string() + "): " + std::string(err.what()), "ui_failure");
        return false;
      }
      return true;
    }

    std::optional<sol::table> try_get_table(sol::state& lua, std::string const& name, message_log& log)
    {
      sol::table t = lua[name];
      if (!t.valid())
      {
        log.add("Lua Error: Table '" + name + "' not found.", "ui_failure");
        return std::nullopt;
      }
      return t;
    }

    entity_data parse_entity_config(sol::table const& t, std::string_view default_name)
    {
      entity_data cfg;

      // Stats Extraction
      cfg.stats.archetype = t.get_or<std::string>("archetype", "Unknown");
      cfg.stats.max_hp = t.get_or("max_hp", t.get_or("hp", 10)); // Allow 'hp' to define max if max_hp missing
      cfg.stats.hp = t.get_or("hp", cfg.stats.max_hp);
      cfg.stats.max_mana = t.get_or("max_mp", t.get_or("mp", 0));
      cfg.stats.mana = t.get_or("mp", cfg.stats.max_mana);
      cfg.stats.damage = t.get_or("damage", 1);
      cfg.stats.xp = 0;
      cfg.stats.level = 1;
      cfg.stats.fov_range = t.get_or("fov", 8);
      cfg.stats.gold = 0;
      cfg.stats.action_delay = t.get_or("delay", 10);
      cfg.stats.action_timer = 0;

      // Renderable Extraction
      std::string glyph_str = t.get_or<std::string>("glyph", "?");
      cfg.render.glyph = glyph_str.empty() ? '?' : glyph_str[0];
      cfg.render.color = t.get_or<std::string>("color", "ui_default");

      // Meta
      cfg.name = t.get_or("name", std::string(default_name));
      cfg.type = t.get_or("type", std::string("entity"));

      return cfg;
    }
  }

  void message_log::add(std::string msg, std::string const& color)
  {
    messages.push_back({msg, color});
    if (messages.size() > max_messages) messages.erase(messages.begin());
  }

  entity_id systems::get_entity_at(registry const& reg, int x, int y, entity_id ignore_id)
  {
    entity_id found = 0;
    for (auto const& [id, pos] : reg.positions)
    {
      if (id == ignore_id) continue;
      if (pos.x == x && pos.y == y)
      {
        if (reg.stats.contains(id)) return id;
        found = id;
      }
    }
    return found;
  }

  void systems::attack(registry& reg, entity_id a_id, entity_id d_id, message_log& log, sol::state& lua)
  {
    auto& a = reg.stats[a_id];
    auto& d = reg.stats[d_id];
    d.hp -= a.damage;

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

  void systems::check_level_up(registry& reg, message_log& log, sol::state& lua)
  {
    auto& s = reg.stats[reg.player_id];
    int next_lvl_xp = s.level * 100;

    if (s.xp >= next_lvl_xp)
    {
      s.level++;
      if (!execute_script(lua, reg.player_class_script, log)) return;

      sol::table current_stats = lua.create_table();
      current_stats["hp"] = s.max_hp;
      current_stats["mp"] = s.max_mana;
      current_stats["damage"] = s.damage;
      current_stats["delay"] = s.action_delay;

      sol::protected_function level_func = lua["level_up"];
      auto level_res = level_func(current_stats);

      if (level_res.valid())
      {
        sol::table new_stats_table = level_res;

        // Use parser to extract updated fields (partially)
        // Note: level_up returns a partial config, so we can re-use the parser
        // effectively treating it as a config object.
        entity_data cfg = parse_entity_config(new_stats_table);

        s.max_hp = cfg.stats.max_hp;
        s.hp = s.max_hp;
        s.max_mana = cfg.stats.max_mana;
        s.mana = s.max_mana;
        s.damage = cfg.stats.damage;
        s.action_delay = cfg.stats.action_delay;

        log.add("Level Up! You are now Level " + std::to_string(s.level), "ui_gold");
      }
    }
  }

  void systems::cast_fireball(registry& reg, dungeon& map, int dx, int dy, message_log& log, sol::state& lua)
  {
    std::string script_path = "scripts/spells/fireball.lua";

    if (!execute_script(lua, script_path, log)) return;

    // Use Helper
    auto data_opt = try_get_table(lua, "spell_data", log);
    if (!data_opt) return;
    sol::table data = *data_opt;

    int mana_cost = data.get_or("mana_cost", 10);
    int damage = data.get_or("damage", 10);
    int range = data.get_or("range", 10);
    int delay = data.get_or("delay", 2);

    std::string glyph_str = data.get_or<std::string>("glyph", "*");
    char glyph = glyph_str.empty() ? '*' : glyph_str[0];
    std::string color = data.get_or<std::string>("color", "ui_default");
    std::string name = data.get_or<std::string>("name", "Spell");

    auto& s = reg.stats[reg.player_id];
    if (s.mana < mana_cost)
    {
      log.add("Not enough mana! (" + std::to_string(mana_cost) + " required)", "ui_emphasis");
      return;
    }
    s.mana -= mana_cost;

    position p = reg.positions[reg.player_id];
    int start_x = p.x + dx;
    int start_y = p.y + dy;

    if (!map.is_walkable(start_x, start_y))
    {
      log.add("You cast a " + name + " into the wall...", "ui_default");
      return;
    }

    entity_id id = reg.create_entity();
    reg.positions[id] = p;
    reg.renderables[id] = {glyph, color};
    reg.projectiles[id] = {dx, dy, damage, range, reg.player_id, delay, 0};
    reg.script_paths[id] = script_path;
    reg.names[id] = name;

    log.add("You cast a " + name + "!", color);
  }

  bool systems::update_projectiles(registry& reg, dungeon const& map, message_log& log, sol::state& lua)
  {
    std::vector<entity_id> to_destroy;
    bool any_change = false;

    for (auto& [id, proj] : reg.projectiles)
    {
      if (!reg.positions.contains(id))
      {
        to_destroy.push_back(id);
        continue;
      }

      if (proj.range < 0)
      {
        to_destroy.push_back(id);
        any_change = true;
        continue;
      }

      if (proj.action_timer > 0)
      {
        proj.action_timer--;
        continue;
      }

      proj.action_timer = proj.action_delay;
      any_change = true;

      if (proj.range == 0)
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

      if (!map.is_walkable(tx, ty))
      {
        log.add(reg.names[id] + " hits a wall.", "ui_default");
        pos.x = tx;
        pos.y = ty;
        proj.range = -1;
        continue;
      }

      entity_id target = get_entity_at(reg, tx, ty, id);
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
        pos.x = tx;
        pos.y = ty;
        proj.range = -1;
        continue;
      }

      pos.x = tx;
      pos.y = ty;
    }

    for (auto id : to_destroy) reg.destroy_entity(id);
    return any_change;
  }

  bool systems::move_monsters(registry& reg, dungeon const& map, message_log& log, sol::state& lua)
  {
    if (!reg.positions.contains(reg.player_id)) return false;
    position p_pos = reg.positions.at(reg.player_id);
    bool any_change = false;

    for (auto m_id : reg.monsters)
    {
      if (!reg.positions.contains(m_id) || !reg.script_paths.contains(m_id)) continue;

      auto& m_stats = reg.stats[m_id];
      if (m_stats.action_timer > 0)
      {
        m_stats.action_timer--;
        continue;
      }

      m_stats.action_timer = m_stats.action_delay;

      auto& m_pos = reg.positions[m_id];
      std::string const& script = reg.script_paths[m_id];

      auto script_res = lua.safe_script_file(systems::checked_script_path(script), sol::script_pass_on_error);
      if (!script_res.valid()) continue;

      sol::protected_function ai_func = lua["update_ai"];
      auto res = ai_func(m_pos.x, m_pos.y, p_pos.x, p_pos.y);

      if (res.valid())
      {
        int dx = res[0], dy = res[1];
        if (dx != 0 || dy != 0)
        {
          int tx = m_pos.x + dx, ty = m_pos.y + dy;
          if (tx == p_pos.x && ty == p_pos.y)
          {
            attack(reg, m_id, reg.player_id, log, lua);
            any_change = true;
          }
          else if (map.is_walkable(tx, ty) && get_entity_at(reg, tx, ty) == 0)
          {
            m_pos.x = tx;
            m_pos.y = ty;
            any_change = true;
          }
        }
      }
    }
    return any_change;
  }
}
