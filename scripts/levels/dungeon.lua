-- scripts/levels/dungeon.lua
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
    if depth % 3 == 0 then
        return "scripts/levels/forest.lua"
    else
        return "scripts/levels/dungeon.lua"
    end
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
        ["scripts/items/healing_potion.lua"] = 80,
        ["scripts/items/strength_potion.lua"] = 20
    }
end