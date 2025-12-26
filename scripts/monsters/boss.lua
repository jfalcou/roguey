function get_init_stats()
    return {
        type = "boss",
        hp = 300,
        max_hp = 300,
        damage = 25,
        glyph = "B",
        color = 10     -- Corresponds to init_pair(10, COLOR_RED, ...) in C++
    }
end

function update_ai(mx, my, px, py)
    -- Boss always moves towards player
    local dx = (px > mx) and 1 or (px < mx and -1 or 0)
    local dy = (py > my) and 1 or (py < my and -1 or 0)
    return dx, dy
end