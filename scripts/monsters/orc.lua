function get_init_stats()
    return {
        type = "minion",
        hp = 30,
        max_hp = 30,
        damage = 8,
        glyph = "O",
        color = Orc
    }
end

function update_ai(mx, my, px, py)
    local dx = (px > mx) and 1 or (px < mx and -1 or 0)
    local dy = (py > my) and 1 or (py < my and -1 or 0)
    return dx, dy
end