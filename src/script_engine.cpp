#include "script_engine.hpp"
#include <iostream>

namespace fs = std::filesystem;

ScriptEngine::ScriptEngine() {
    // Constructor intentionally empty, explicit init called by Game
}

void ScriptEngine::init_lua() {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

    // Register basic types
    lua.new_usertype<Position>("Position", "x", &Position::x, "y", &Position::y);
    lua.new_usertype<Stats>("Stats",
        "hp", &Stats::hp, "max_hp", &Stats::max_hp,
        "mana", &Stats::mana, "max_mana", &Stats::max_mana,
        "damage", &Stats::damage, "xp", &Stats::xp, "level", &Stats::level);

    // We register MessageLog in Game because it depends on ncurses logic,
    // or we can pass a lambda to Lua in Game::init_lua if we keep the registration there.
    // For now, let's keep the ColorPair enum registration here.

    lua.new_enum<ColorPair>("ColorPair", {
        {"Default", ColorPair::Default},
        {"Player", ColorPair::Player},
        {"Wall", ColorPair::Wall},
        // ... (Lua uses the global variables injected by Game for names, but this enum is good for type safety)
    });
}

void ScriptEngine::discover_assets() {
    monster_templates.clear(); class_templates.clear(); item_templates.clear();
    if (fs::exists("scripts/monsters")) {
        for (const auto& entry : fs::directory_iterator("scripts/monsters"))
            if (entry.path().extension() == ".lua") monster_templates.push_back(entry.path().string());
    }
    if (fs::exists("scripts/class")) {
        for (const auto& entry : fs::directory_iterator("scripts/class"))
            if (entry.path().extension() == ".lua") class_templates.push_back(entry.path().string());
    }
    if (fs::exists("scripts/items")) {
        for (const auto& entry : fs::directory_iterator("scripts/items"))
            if (entry.path().extension() == ".lua") item_templates.push_back(entry.path().string());
    }
}

bool ScriptEngine::load_script(const std::string& path) {
    auto res = lua.safe_script_file(path, sol::script_pass_on_error);
    return res.valid();
}

std::string ScriptEngine::pick_from_weights(sol::table weights, std::mt19937& gen) {
    int total_weight = 0;
    std::vector<std::pair<std::string, int>> pool;
    for (auto const& [key, val] : weights) {
        int w = val.as<int>();
        total_weight += w;
        pool.push_back({key.as<std::string>(), total_weight});
    }
    if (total_weight <= 0) return "";
    int roll = std::uniform_int_distribution<>(1, total_weight)(gen);
    for (auto const& p : pool) { if (roll <= p.second) return p.first; }
    return "";
}