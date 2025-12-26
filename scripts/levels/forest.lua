-- scripts/levels/forest.lua
function get_level_config(depth)
    return {
        name = "Whispering Woods",
        width = 80,
        height = 20,
        wall_color = Forest,   -- Used as a global variable
        floor_color = Default, -- Used as a global variable
        is_boss_level = (depth % 5 == 0)
    }
end

function get_next_level(depth)
    return "scripts/levels/dungeon.lua"
end

function get_spawn_odds(depth)
    return { ["scripts/monsters/slime.lua"] = 100 }
end

function get_loot_odds(depth)
    return { ["scripts/items/healing_potion.lua"] = 100 }
end