--==================================================================================================
--  Roguey
--  Copyright : Joel FALCOU
--  SPDX-License-Identifier: MIT
--==================================================================================================

game_colors = {
    -- { FG, BG, "GlobalName" }
    { 7, 0, "Default" },
    { 6, 0, "Player" },
    { 1, 0, "Orc" },
    { 7, 0, "Wall" },
    { 4, 0, "Spell" },
    { 3, 0, "Gold" },
    { 5, 0, "Boss" },
    { 0, 0, "Hidden" },
    { 1, 0, "HP_Potion" },
    { 5, 0, "STR_Potion" },
    { 2, 0, "Forest" },
    { 3, 2, "Slime" }
}

help_text = [[
  CONTROLS
  --------
  Arrow Keys : Move / Attack
  i          : Open Inventory
  c          : Character Stats
  f          : Cast Fireball
  ESC        : Open Help / Close Menus
  q          : Quit Game
]]

function get_start_config()
    return {
        start_level = "scripts/levels/forest.lua",
        start_depth = 1,
        initial_log_message = "Welcome adventurer !",
        window_width = 100,
        window_height = 48
    }
end
