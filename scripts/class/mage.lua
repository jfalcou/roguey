function get_init_stats()
    return { hp = 80, mp = 60, damage = 5 }
end

function level_up(stats)
    return {
        hp = stats.hp + 10,    -- Fragile
        mp = stats.mp + 20,    -- High mana gain
        damage = stats.damage + 2
    }
end