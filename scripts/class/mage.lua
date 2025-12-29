--==================================================================================================
--  Roguey
--  Copyright : Joel FALCOU
--  SPDX-License-Identifier: MIT
--==================================================================================================

function get_init_stats()
    return { archetype = "Mage", hp = 80, mp = 60, damage = 5, delay = 3 }
end

function level_up(stats)
    return {
        hp = stats.hp + 10,
        mp = stats.mp + 20,
        damage = stats.damage + 2
    }
end