item_data = {
    name = "Strength Elixir",
    glyph = "!",
    color = item_dmg_potion
}

function on_use(stats, log)
    stats.damage = stats.damage + 2
    log:add("Your muscles bulge! Damage increased!")
    return true
end