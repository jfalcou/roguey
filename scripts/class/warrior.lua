function get_init_stats()
    return { hp = 150, mp = 10, damage = 15 }
end

function level_up(stats)
    return {
        hp = stats.hp + 30,    -- Warriors gain lots of HP
        mp = stats.mp + 2,     -- Low mana gain
        damage = stats.damage + 5
    }
end