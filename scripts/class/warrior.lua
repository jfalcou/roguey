--==================================================================================================
--  Roguey
--  Copyright : Joel FALCOU
--  SPDX-License-Identifier: MIT
--==================================================================================================

function get_init_stats()
    return { hp = 150, mp = 10, damage = 15 }
end

function level_up(stats)
    return {
        hp = stats.hp + 30,
        mp = stats.mp + 2,
        damage = stats.damage + 5
    }
end