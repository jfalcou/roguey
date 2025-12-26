-- scripts/levels/dungeon.lua
function get_level_config(depth)
    return {
        name = "Whispering Woods",
        width = 80,
        height = 20,
        wall_color = Wall,
        floor_color = Default,
        is_boss_level = (depth % 3 == 0)
    }
end
function get_next_level(depth)
    -- Logic to decide where the stairs lead
    if depth % 3 == 0 then
        return "scripts/levels/forest.lua" -- After boss, go to forest
    else
        return "scripts/levels/dungeon.lua" -- Keep going deeper
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
        ["scripts/items/strength_elixir.lua"] = 20
    }
end