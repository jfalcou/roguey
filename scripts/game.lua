--==================================================================================================
--  Roguey
--  Copyright : Joel FALCOU
--  SPDX-License-Identifier: MIT
--==================================================================================================

game_colors = {
-- UI COLOR
    { 7, 0, false, "ui_default" }
  , { 7, 0, true , "ui_border"  }
  , { 8, 0, true , "ui_hidden"  }
  , { 1, 0, true , "ui_failure" }
  , { 3, 0, true , "ui_gold"    }
  , { 1, 0, true , "ui_emphasis"}
-- ASSET COLOR
  , { 7, 0, false, "asset_floor" }
  , { 3, 0, false, "asset_dirt" }
  , { 7, 0, false, "asset_wall" }
  , { 2, 0, false, "asset_tree" }
-- ENTITY COLOR
  , { 6, 0, false, "entity_player" }
  , { 1, 0, false, "entity_orc" }
  , { 5, 0, false, "entity_boss" }
  , { 2, 3, true , "entity_slime" }
-- ITEMS
  , { 1, 0, true , "item_hp_potion" }
  , { 5, 0, false, "item_dmg_potion" }
-- FX COLOR
  , { 4, 0, true , "fx_fire" }
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
