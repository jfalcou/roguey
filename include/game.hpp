#pragma once
#include "registry.hpp"
#include "dungeon.hpp"
#include "systems.hpp"
#include "script_engine.hpp" // Added
#include <vector>

class Game {
    Registry reg;
    Dungeon map;
    MessageLog log;
    ScriptEngine scripts;

    bool running = true;
    bool debug_mode = false; // Added for -d flag
    GameState state = GameState::Dungeon;
    int last_dx = 1, last_dy = 0;
    int depth = 1;
    std::string current_level_script;
    std::vector<ItemTag> inventory;

public:
    Game(bool debug = false); // Modified constructor
    ~Game();
    void run();

private:
    void reset(bool full_reset, std::string level_script = "");
    void spawn_monster(int x, int y, std::string script_path);
    void spawn_item(int x, int y, std::string script_path);
    void process_input();
    void render();
    void render_dungeon();
    void render_inventory();
    void render_stats();
    void handle_input_inventory(int ch);
    void get_player_setup();
    bool handle_game_over();
    bool handle_victory();
};