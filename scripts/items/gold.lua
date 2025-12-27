--==================================================================================================
--  Roguey
--  Copyright : Joel FALCOU
--  SPDX-License-Identifier: MIT
--==================================================================================================

item_data = {
    name = "Gold",
    glyph = "$",
    color = item_gold,
    kind  = "gold"
}

function on_pick(stats, log)
    local stash = roll("5d4")
    stats.gold = stats.gold + stash
    log:add("You found " .. tostring(stash) .. " gold!")
    return false
end