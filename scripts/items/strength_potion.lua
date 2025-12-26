item_data = {
    name = "Strength Elixir",
    glyph = "!",
    color = STR_Potion
}

function on_use(stats, log)
    stats.damage = stats.damage + 2
    log:add("Your muscles bulge! Damage increased!")
    return true
end