-- scripts/monsters/slime.lua

-- scripts/monsters/slime.lua
function get_init_stats()
    return {
        hp = 10,
        damage = 2,
        glyph = "s",     -- Ensure this is NOT empty
        color = Slime,  -- Ensure 'Forest' is defined in game_colors in game.lua
        type = "minion"
    }
end

function update_ai(mx, my, px, py)
    local dx, dy = 0, 0
    -- Simple greedy pathfinding towards the player
    if mx < px then dx = 1 elseif mx > px then dx = -1 end
    if my < py then dy = 1 elseif my > py then dy = -1 end

    return dx, dy
end