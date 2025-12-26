#include "game.hpp"
#include <ncurses.h>
#include <filesystem>
#include <random>
#include <map>

namespace fs = std::filesystem;

// Constructor: Renderer initializes ncurses automatically
Game::Game(bool debug) : map(80, 20), debug_mode(debug) {
    scripts.init_lua();

    // 1. Bind MessageLog (so 'log:add' works)
    scripts.lua.new_usertype<MessageLog>("Log", "add", [](MessageLog& l, std::string msg) { l.add(msg, ColorPair::Default); });

    if (!scripts.load_script("scripts/game.lua")) {
        renderer.show_error("Failed to load 'scripts/game.lua'.\nCheck that the file exists and is valid.");
        exit(1);
    }

    renderer.init_colors(scripts);

    sol::protected_function start_cfg_func = scripts.lua["get_start_config"];
    sol::table start_config = start_cfg_func();

    scripts.discover_assets();
    get_player_setup();

    std::string first_level = start_config["start_level"];
    log.add(start_config["initial_log_message"]);
    reset(true, first_level);
}

Game::~Game()
{}

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

    std::string glyph_str = s["glyph"].get<std::string>();
    char glyph = glyph_str.empty() ? '?' : glyph_str[0];

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
        last_dx = 1; last_dy = 0;
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

    scripts.load_script(current_level_script);
    sol::table config = scripts.lua["get_level_config"](depth);

    map.width = config["width"];
    map.height = config["height"];
    map.generate();

    reg.player_id = reg.create_entity();
    reg.positions[reg.player_id] = map.rooms[0].center();
    reg.renderables[reg.player_id] = {'@', ColorPair::Player};

    if (full_reset) {
        scripts.load_script(reg.player_class_script);
        sol::protected_function init_func = scripts.lua["get_init_stats"];
        sol::table s = init_func();
        reg.stats[reg.player_id] = { s["hp"], s["hp"], s["mp"], s["mp"], s["damage"], 0, 1, 8 };
    } else {
        reg.stats[reg.player_id] = saved_stats;
    }

    std::random_device rd; std::mt19937 gen(rd());
    std::string next_level_path = scripts.lua["get_next_level"](depth);

    if (config["is_boss_level"].get_or(false)) {
        spawn_monster(map.rooms.back().center().x, map.rooms.back().center().y, scripts.lua["get_boss_script"](depth));
    } else {
        EntityID stairs = reg.create_entity();
        reg.positions[stairs] = map.rooms.back().center();
        reg.renderables[stairs] = {'>', ColorPair::Gold};
        reg.items[stairs] = {ItemType::Stairs, 0, next_level_path, ""};
    }

    sol::table monster_weights = scripts.lua["get_spawn_odds"](depth);
    sol::table item_weights = scripts.lua["get_loot_odds"](depth);

    std::map<std::string, int> spawn_counts;

    for (size_t i = 1; i < map.rooms.size() - 1; ++i) {
        Position c = map.rooms[i].center();
        int roll = std::uniform_int_distribution<>(0, 10)(gen);
        if (roll < 3) {
            std::string path = scripts.pick_from_weights(item_weights, gen);
            if (!path.empty()) spawn_item(c.x, c.y, path);
        } else if (roll < 7) {
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
            if (handle_game_over()) {
                sol::table config = scripts.lua["get_start_config"]();
                std::string start_level = config["start_level"];
                log.add(config["initial_log_message"]);

                reset(true, start_level);
                continue;
            } else break;
        }
        if (reg.boss_id != 0 && !reg.positions.contains(reg.boss_id)) {
            if (handle_victory()) {
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
        else if (ch == KEY_UP)    { dy = -1; acted = true; }
        else if (ch == KEY_DOWN)  { dy = +1; acted = true; }
        else if (ch == KEY_LEFT)  { dx = -1; acted = true; }
        else if (ch == KEY_RIGHT) { dx = +1; acted = true; }
        else if (ch == 'f' || ch == 'F') {
            Systems::cast_fireball(reg, map, last_dx, last_dy, log, renderer);
            acted = true;
        }

        if (acted) {
            if (dx != 0 || dy != 0) {
                last_dx = dx;
                last_dy = dy;
            }

            Position& p = reg.positions[reg.player_id];
            EntityID target = Systems::get_entity_at(reg, p.x + dx, p.y + dy);

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
            Systems::move_monsters(reg, map, log, scripts.lua);
            map.update_fov(p.x, p.y, reg.stats[reg.player_id].fov_range);
        }
    } else if (state == GameState::Inventory) {
        handle_input_inventory(ch);
    }
}

void Game::render() {
    renderer.clear_screen();

    if (state == GameState::Dungeon) {
        sol::table config = scripts.lua["get_level_config"](depth);
        int wc = config["wall_color"].get_or(4);
        int fc = config["floor_color"].get_or(1);

        renderer.draw_dungeon(map, reg, log, reg.player_id, depth, wc, fc);
    }
    else if (state == GameState::Inventory) {
        renderer.draw_inventory(inventory);
    }
    else if (state == GameState::Stats) {
        renderer.draw_stats(reg, reg.player_id, reg.player_name);
    }

    renderer.refresh_screen();
}

void Game::handle_input_inventory(int ch) {
    if (ch >= '1' && ch <= '9') {
        size_t idx = ch - '1';
        if (idx < inventory.size()) {
            auto& item = inventory[idx];
            if (scripts.load_script(item.script)) {
                sol::protected_function on_use = scripts.lua["on_use"];

                // FIXED: Capture the result and check for errors
                auto use_res = on_use(reg.stats[reg.player_id], log);

                if (use_res.valid()) {
                    bool keep_item = false;
                    // Optional: If lua returns true/false, we could decide to keep item?
                    // For now, we assume use = consume.
                    inventory.erase(inventory.begin() + idx);
                    state = GameState::Dungeon;
                } else {
                    // CRITICAL: Print Lua error if potion fails
                    sol::error err = use_res;
                    log.add("Script Error: " + std::string(err.what()), ColorPair::Orc);
                }
            }
        }
    }
}

bool Game::handle_game_over() {
    flushinp();
    clear();
    attron(A_BOLD | COLOR_PAIR(3));
    mvprintw(10, 30, " !!! YOU DIED !!! ");
    attroff(A_BOLD | COLOR_PAIR(3));
    mvprintw(15, 20, "Press 'r' to Restart, 'q' to Quit");
    refresh();

    while (true) {
        int choice = getch();
        if (choice == 'r' || choice == 'R') return true;
        if (choice == 'q' || choice == 'Q') return false;
    }
}

bool Game::handle_victory() {
    flushinp();
    clear();
    attron(A_BOLD | COLOR_PAIR(6));
    mvprintw(10, 30, " !!! VICTORY !!! ");
    attroff(A_BOLD | COLOR_PAIR(6));
    mvprintw(15, 20, "Press 'c' to Continue, 'q' to Quit");
    refresh();

    while (true) {
        int choice = getch();
        if (choice == 'c' || choice == 'C') return true;
        if (choice == 'q' || choice == 'Q') return false;
    }
}
