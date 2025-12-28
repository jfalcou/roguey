--==================================================================================================
--  Roguey
--  Copyright : Joel FALCOU
--  SPDX-License-Identifier: MIT
--==================================================================================================

game_colors = {
    -- UI Colors
    ui_default    = "#DCDCDC",
    ui_border     = "#6495ED",
    ui_hidden     = "#3C3C3C",
    ui_failure    = { fg = "#FFFFFF", bg = "#DC143C" },

    ui_gold       = "#FFD700",
    ui_emphasis   = "#FF8C00",

    -- Assets
    asset_floor   = "#282828",
    asset_dirt    = "#8B4513",
    asset_wall    = "#778899",
    asset_tree    = "#228B22",

    -- Entities
    entity_player = "#00FFFF",
    entity_orc    = "#9ACD32",
    entity_boss   = "#9400D3",
    entity_slime  = { fg = "#9ACD32", bg = "#055315" },

    -- Items
    item_gold       = "#FFD700",
    item_hp_potion  = "#FF6347",
    item_dmg_potion = "#8A2BE2",

    -- FX
    fx_fire         = "#FF4500"
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
        initial_log_message = "Welcome adventurer !"
    }
end