--==================================================================================================
--  Roguey
--  Copyright : Joel FALCOU
--  SPDX-License-Identifier: MIT
--==================================================================================================

function get_level_config(depth)
    return {
        name = "Deep Dungeon",
        width = 120,
        height = 40,
        wall_color = asset_wall,
        floor_color = asset_floor,
        is_boss_level = (depth % 3 == 0)
    }
end

function get_next_level(depth)
    return "scripts/levels/dungeon.lua"
end

function get_boss_script(depth)
    return "scripts/monsters/boss.lua"
end

function get_spawn_odds(depth)
    return {
        ["scripts/monsters/slime.lua"] = 50,
        ["scripts/monsters/orc.lua"] = 30 + (depth * 2)
    }
end

function get_loot_odds(depth)
    return {
        ["scripts/items/healing_potion.lua"] = 60,
        ["scripts/items/healing_potion.lua"] = 10,
        ["scripts/items/gold.lua"] = 30
    }
end