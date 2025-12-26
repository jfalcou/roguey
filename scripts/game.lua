-- scripts/game.lua
game_colors = {
    -- { FG, BG, "GlobalName" }
    { 7, 0, "Default" },
    { 6, 0, "Player" },
    { 1, 0, "Orc" },
    { 7, 0, "Wall" },
    { 4, 0, "Spell" },
    { 3, 0, "Gold" },
    { 5, 0, "Boss" },
    { 0, 0, "Hidden" },
    { 1, 0, "HP_Potion" },
    { 5, 0, "STR_Potion" },
    { 2, 0, "Forest" },
    { 3, 2, "Slime" }
}

function get_start_config()
    return {
        start_level = "scripts/levels/forest.lua",
        start_depth = 1,
        initial_log_message = "Engine ready. Auto-ID Color bridge active."
    }
end