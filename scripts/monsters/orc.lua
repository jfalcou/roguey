function get_init_stats()
    return {
        color = Orc,
        damage = 8,
        glyph = "O",
        hp = 30,
        max_hp = 30,
        type = "minion"
    }
end

function update_ai(mx, my, px, py)
    local dx = (px > mx) and 1 or (px < mx and -1 or 0)
    local dy = (py > my) and 1 or (py < my and -1 or 0)
    return dx, dy
end