#pragma once
#include <vector>
#include <string>
#include "dungeon.hpp"
#include "registry.hpp"
#include "script_engine.hpp"
#include "systems.hpp" // For MessageLog

// Assuming ItemTag is defined in types.hpp or similar.
// If it was internal to Game, we might need to move it to types.hpp.
// For now, we will forward declare or include types.
#include "types.hpp"

class Renderer {
public:
    Renderer();
    ~Renderer();

    // loads colors from Lua "game_colors"
    void init_colors(ScriptEngine& scripts);

    void clear_screen();
    void refresh_screen();
    void show_error(const std::string& msg);

    void animate_projectile(int x, int y, char glyph, ColorPair color);

    void draw_log(const MessageLog& log, int start_y, int max_row, int max_col);
    void draw_dungeon ( const Dungeon& map, const Registry& reg, const MessageLog& log
                      , int player_id, int depth, int wall_color, int floor_color
                      );
    void draw_borders(int width, int height);
    void draw_inventory(const std::vector<ItemTag>& inventory);
    void draw_stats(const Registry& reg, int player_id, std::string player_name);
};