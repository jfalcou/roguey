function get_init_stats()
    return {
        color = entity_boss,
        damage = 25,
        glyph = "B",
        hp = 300,
        type = "boss"
    }
end

function update_ai(mx, my, px, py)
    -- entity_boss always moves towards player
    local dx = (px > mx) and 1 or (px < mx and -1 or 0)
    local dy = (py > my) and 1 or (py < my and -1 or 0)
    return dx, dy
end
