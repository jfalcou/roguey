item_data = {
    name = "Healing Potion",
    glyph = "!",
    color = item_hp_potion
}

function on_use(stats, log)
    local heal_amount = 20
    stats.hp = math.min(stats.max_hp, stats.hp + heal_amount)
    log:add("You drink the potion and feel better!")
    return true
end