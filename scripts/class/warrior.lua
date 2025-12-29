--==================================================================================================
--  Roguey
--  Copyright : Joel FALCOU
--  SPDX-License-Identifier: MIT
--==================================================================================================

function get_init_stats()
    return { archetype = "Warrior", hp = 150, mp = 10, damage = 15, delay = 4 }
end

function level_up(stats)
    return {
        hp = stats.hp + roll("2d10+5"),
        mp = stats.mp + roll("d4"),
        damage = stats.damage + roll("1D6")
    }
end