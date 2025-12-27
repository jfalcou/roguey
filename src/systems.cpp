#include "renderer.hpp"
#include "systems.hpp"
#include <algorithm>
#include <cmath>

void MessageLog::add(std::string msg, std::string const& color)
{
  messages.push_back({msg, color});
  if (messages.size() > max_messages) messages.erase(messages.begin());
}

EntityID Systems::get_entity_at(Registry const& reg, int x, int y)
{
  for (auto const& [id, pos] : reg.positions)
  {
    if (pos.x == x && pos.y == y) return id;
  }
  return 0;
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
    else if (d_id == reg.player_id)
    {
      // NEW: Requested death log
      log.add(d_name + " was defeated by the " + a_name, "ui_emphasis");
    }
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
    auto res = lua.safe_script_file(reg.player_class_script, sol::script_pass_on_error);
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

void Systems::cast_fireball(Registry& reg, Dungeon& map, int dx, int dy, MessageLog& log, Renderer& renderer)
{
  auto& s = reg.stats[reg.player_id];
  if (s.mana < 20)
  {
    log.add("Not enough mana!", "ui_emphasis");
    return;
  }
  s.mana -= 20;
  Position p = reg.positions[reg.player_id];

  for (int i = 1; i <= 6; ++i)
  {
    int tx = p.x + dx * i;
    int ty = p.y + dy * i;

    renderer.animate_projectile(tx, ty, '*', "fx_fire");

    if (!map.is_walkable(tx, ty))
    {
      log.add("Fireball hits a wall.", "ui_default");
      break;
    }

    EntityID target = get_entity_at(reg, tx, ty);
    if (target && target != reg.player_id)
    {
      reg.stats[target].hp -= 40;
      std::string t_name = reg.names.count(target) ? reg.names.at(target) : "Target";
      log.add("Fireball burns " + t_name + "!", "fx_fire");

      if (reg.stats[target].hp <= 0)
      {
        log.add(t_name + " incinerated.", "ui_gold");
        reg.destroy_entity(target);
      }
      break;
    }
  }
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

    auto script_res = lua.safe_script_file(script, sol::script_pass_on_error);
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
