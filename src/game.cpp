#include "game.hpp"
#include <ncurses.h>
#include <filesystem>
#include <random>
#include <map>

namespace fs = std::filesystem;
Game::Game(bool debug) : map(80, 20), debug_mode(debug) {
    initscr();
    start_color();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    // Initialize Script Engine
    scripts.init_lua();
    // Register the Log type here specifically since it relies on private Game logic/UI
    scripts.lua.new_usertype<MessageLog>("Log", "add", [](MessageLog& l, std::string msg) { l.add(msg, ColorPair::Default); });

    // 1. Load game.lua
    if (!scripts.load_script("scripts/game.lua")) {
        clear();
        attron(A_BOLD | A_REVERSE);
        mvprintw(10, 20, " ! FATAL ERROR ! ");
        mvprintw(12, 10, "scripts/game.lua is missing or broken.");
        refresh(); getch(); endwin(); exit(1);
    }

    // 2. AUTO-INDEXING COLOR SYSTEM
    sol::table color_table = scripts.lua["game_colors"];
    for (size_t i = 1; i <= color_table.size(); ++i) {
        sol::table entry = color_table[i];

        int id = static_cast<int>(i);
        int fg = entry[1];
        int bg = entry[2];
        std::string name = entry[3];

        init_pair(id, fg, bg);
        scripts.lua[name] = id;
    }

    sol::protected_function start_cfg_func = scripts.lua["get_start_config"];
    sol::table start_config = start_cfg_func();

    scripts.discover_assets(); // Call the engine's discovery
    get_player_setup();

    std::string first_level = start_config["start_level"];
    log.add(start_config["initial_log_message"]);
    reset(true, first_level);
}

Game::~Game() { endwin(); }

void Game::get_player_setup() {
    clear(); echo(); curs_set(1);
    mvprintw(5, 10, "--- CHARACTER CREATION ---");
    mvprintw(7, 10, "Enter your name: ");
    refresh();
    char name_buf[32]; getnstr(name_buf, 31);
    reg.player_name = std::string(name_buf).empty() ? "Hero" : name_buf;
    noecho(); curs_set(0);

    int selection = 0;
    while (true) {
        clear();
        mvprintw(5, 10, "--- SELECT YOUR CLASS ---");
        // FIX: Access class_templates via the 'scripts' engine instance
        for (size_t i = 0; i < scripts.class_templates.size(); ++i) {
            if ((int)i == selection) attron(A_REVERSE);
            mvprintw(7 + i, 12, "[ %s ]", fs::path(scripts.class_templates[i]).stem().string().c_str());
            attroff(A_REVERSE);
        }
        int ch = getch();
        if (ch == KEY_UP && selection > 0) selection--;
        if (ch == KEY_DOWN && selection < (int)scripts.class_templates.size() - 1) selection++;
        if (ch == 10 || ch == 13 || ch == ' ') {
            reg.player_class_script = scripts.class_templates[selection];
            break;
        }
    }
}

void Game::spawn_item(int x, int y, std::string script_path) {
if (!scripts.load_script(script_path)) return;
    EntityID id = reg.create_entity();
    reg.positions[id] = {x, y};
    sol::table data = scripts.lua["item_data"];
    std::string glyph_str = data["glyph"];
    reg.renderables[id] = { glyph_str[0], static_cast<ColorPair>(data["color"].get<int>()) };
    reg.items[id] = { ItemType::Consumable, 0, data["name"], script_path };
}

void Game::spawn_monster(int x, int y, std::string script_path) {
    if (!scripts.load_script(script_path)) return;

    EntityID id = reg.create_entity();
    reg.positions[id] = {x, y};
    reg.script_paths[id] = script_path;
    reg.monsters.push_back(id);

    sol::protected_function init_func = scripts.lua["get_init_stats"];
    sol::table s = init_func();
    reg.stats[id] = { s["hp"], s["hp"], 0, 0, s["damage"], 0, 1, 0 };

    // FIX: Safely convert string glyph to char
    std::string glyph_str = s["glyph"].get<std::string>();
    char glyph = glyph_str.empty() ? '?' : glyph_str[0];

    // FIX: Safely convert color to int (prevent invisible monsters)
    int cid = s["color"].get<int>();
    reg.renderables[id] = { glyph, static_cast<ColorPair>(cid == 0 ? 1 : cid) };

    std::string m_type = s["type"];
    if (m_type == "boss") reg.boss_id = id;
}

void Game::reset(bool full_reset, std::string level_script) {
    Stats saved_stats;
    if (!full_reset) {
        saved_stats = reg.stats[reg.player_id];
        depth++;
    } else {
        depth = 1;
        inventory.clear();
    }

    if (!level_script.empty()) current_level_script = level_script;

    reg.positions.clear();
    reg.renderables.clear();
    reg.items.clear();
    reg.stats.clear();
    reg.script_paths.clear();
    reg.monster_types.clear();
    reg.monsters.clear();
    reg.boss_id = 0;
    reg.next_id = 1;
    state = GameState::Dungeon;

    // FIX: Use scripts engine
    scripts.load_script(current_level_script);
    sol::table config = scripts.lua["get_level_config"](depth);

    map.width = config["width"];
    map.height = config["height"];
    map.generate();

    reg.player_id = reg.create_entity();
    reg.positions[reg.player_id] = map.rooms[0].center();
    reg.renderables[reg.player_id] = {'@', ColorPair::Player};

    if (full_reset) {
        // FIX: Use scripts engine
        scripts.load_script(reg.player_class_script);
        sol::protected_function init_func = scripts.lua["get_init_stats"];
        sol::table s = init_func();
        reg.stats[reg.player_id] = { s["hp"], s["hp"], s["mp"], s["mp"], s["damage"], 0, 1, 8 };
    } else {
        reg.stats[reg.player_id] = saved_stats;
    }

    std::random_device rd; std::mt19937 gen(rd());
    // FIX: Use scripts.lua
    std::string next_level_path = scripts.lua["get_next_level"](depth);

    if (config["is_boss_level"].get_or(false)) {
        // FIX: Use scripts.lua
        spawn_monster(map.rooms.back().center().x, map.rooms.back().center().y, scripts.lua["get_boss_script"](depth));
    } else {
        EntityID stairs = reg.create_entity();
        reg.positions[stairs] = map.rooms.back().center();
        reg.renderables[stairs] = {'>', ColorPair::Gold};
        reg.items[stairs] = {ItemType::Stairs, 0, next_level_path, ""};
    }

    // FIX: Use scripts.lua
    sol::table monster_weights = scripts.lua["get_spawn_odds"](depth);
    sol::table item_weights = scripts.lua["get_loot_odds"](depth);

    std::map<std::string, int> spawn_counts;

    for (size_t i = 1; i < map.rooms.size() - 1; ++i) {
        Position c = map.rooms[i].center();
        int roll = std::uniform_int_distribution<>(0, 10)(gen);
        if (roll < 3) {
            // FIX: Use scripts helper
            std::string path = scripts.pick_from_weights(item_weights, gen);
            if (!path.empty()) spawn_item(c.x, c.y, path);
        } else if (roll < 7) {
            // FIX: Use scripts helper
            std::string path = scripts.pick_from_weights(monster_weights, gen);
            if (!path.empty()) {
                spawn_monster(c.x, c.y, path);
                spawn_counts[fs::path(path).stem().string()]++;
            }
        }
    }

    if (debug_mode) {
        std::string debug_msg = "Debug Spawn: ";
        for (auto const& [name, count] : spawn_counts) {
            debug_msg += name + " x" + std::to_string(count) + " ";
        }
        log.add(debug_msg, ColorPair::Gold);
    }

    map.update_fov(reg.positions[reg.player_id].x, reg.positions[reg.player_id].y, 8);
}

void Game::run() {
    while(running) {
        if (reg.stats[reg.player_id].hp <= 0) {
            if (handle_game_over()) { reset(true, ""); continue; } else break;
        }
        if (reg.boss_id != 0 && !reg.positions.contains(reg.boss_id)) {
            if (handle_victory()) {
                // FIX: Use scripts engine
                scripts.load_script(current_level_script);
                reset(false, scripts.lua["get_next_level"](depth));
                continue;
            } else break;
        }
        render();
        process_input();
    }
}

void Game::process_input() {
    int ch = getch();
    if (ch == 'i' || ch == 'I') { state = (state == GameState::Inventory) ? GameState::Dungeon : GameState::Inventory; return; }
    if (ch == 'c' || ch == 'C') { state = (state == GameState::Stats) ? GameState::Dungeon : GameState::Stats; return; }
    if (ch == 27) { state = GameState::Dungeon; return; }

    if (state == GameState::Dungeon) {
        int dx = 0, dy = 0; bool acted = false;
        if (ch == 'q') running = false;
        else if (ch == KEY_UP) { dy = -1; acted = true; }
        else if (ch == KEY_DOWN) { dy = 1; acted = true; }
        else if (ch == KEY_LEFT) { dx = -1; acted = true; }
        else if (ch == KEY_RIGHT) { dx = 1; acted = true; }

        if (acted) {
            Position& p = reg.positions[reg.player_id];
            EntityID target = Systems::get_entity_at(reg, p.x + dx, p.y + dy);

            // FIX: Pass scripts.lua
            if (target && reg.stats.contains(target)) {
                Systems::attack(reg, reg.player_id, target, log, scripts.lua);
            } else if (map.is_walkable(p.x + dx, p.y + dy)) {
                p.x += dx; p.y += dy;
                if (target && reg.items.contains(target)) {
                    if (reg.items[target].type == ItemType::Stairs) {
                        reset(false, reg.items[target].name);
                        return;
                    }
                    inventory.push_back(reg.items[target]);
                    log.add("Picked up " + reg.items[target].name);
                    reg.destroy_entity(target);
                }
            }
            // FIX: Pass scripts.lua
            Systems::move_monsters(reg, map, log, scripts.lua);
            map.update_fov(p.x, p.y, reg.stats[reg.player_id].fov_range);
        }
    } else if (state == GameState::Inventory) {
        handle_input_inventory(ch);
    }
}

void Game::render() {
    clear();
    if (state == GameState::Dungeon) render_dungeon();
    else if (state == GameState::Inventory) render_inventory();
    else if (state == GameState::Stats) render_stats();
    refresh();
}

void Game::render_dungeon() {
    scripts.load_script(current_level_script);
    sol::table config = scripts.lua["get_level_config"](depth);

    int wall_pair = config["wall_color"];
    int floor_pair = config["floor_color"];

    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            if (map.visible_tiles.count({x, y})) {
                attron(COLOR_PAIR(map.grid[y][x] == '#' ? wall_pair : floor_pair));
                mvaddch(y, x, map.grid[y][x]);
                attroff(COLOR_PAIR(map.grid[y][x] == '#' ? wall_pair : floor_pair));
            } else if (map.explored[y][x]) {
                attron(COLOR_PAIR(8));
                mvaddch(y, x, map.grid[y][x]);
                attroff(COLOR_PAIR(8));
            }
        }
    }
    for (auto const& [id, r] : reg.renderables) {
        Position p = reg.positions[id];
        if (id == reg.player_id || map.visible_tiles.count(p)) {
            attron(COLOR_PAIR((short)r.color)); mvaddch(p.y, p.x, r.glyph); attroff(COLOR_PAIR((short)r.color));
        }
    }
    auto s = reg.stats[reg.player_id];
    mvprintw(21, 0, "HP: %d/%d | MP: %d/%d | Depth: %d | [I]nv [C]har", s.hp, s.max_hp, s.mana, s.max_mana, depth);
    log.draw(22);
}

void Game::render_inventory() {
    mvprintw(2, 10, "--- INVENTORY ---");
    if (inventory.empty()) mvprintw(5, 12, "(Empty)");
    else { for (size_t i = 0; i < inventory.size(); ++i) mvprintw(5 + i, 12, "%zu. %s", i + 1, inventory[i].name.c_str()); }
    mvprintw(18, 10, "[1-9] Use | [ESC] Exit");
}

void Game::render_stats() {
    auto s = reg.stats[reg.player_id];
    mvprintw(2, 10, "--- STATS ---");
    mvprintw(5, 12, "Name:   %s", reg.player_name.c_str());
    mvprintw(6, 12, "Level:  %d", s.level);
    mvprintw(7, 12, "Damage: %d", s.damage);
    mvprintw(18, 10, "[ESC] Exit");
}

void Game::handle_input_inventory(int ch) {
    if (ch >= '1' && ch <= '9') {
        size_t idx = ch - '1';
        if (idx < inventory.size()) {
            auto& item = inventory[idx];
            if (scripts.load_script(item.script)) {
                sol::protected_function on_use = scripts.lua["on_use"];
                auto use_res = on_use(reg.stats[reg.player_id], log);
                if (use_res.valid()) {
                    inventory.erase(inventory.begin() + idx);
                    state = GameState::Dungeon;
                }
            }
        }
    }
}

bool Game::handle_game_over() {
    clear(); attron(A_BOLD | COLOR_PAIR(3)); mvprintw(10, 30, " !!! YOU DIED !!! ");
    mvprintw(15, 20, "Press 'r' to Restart"); refresh();
    int choice = getch(); return (choice == 'r' || choice == 'R');
}

bool Game::handle_victory() {
    clear(); attron(A_BOLD | COLOR_PAIR(6)); mvprintw(10, 30, " !!! VICTORY !!! ");
    mvprintw(15, 20, "Press 'r' to Continue"); refresh();
    int choice = getch(); return (choice == 'r' || choice == 'R');
}