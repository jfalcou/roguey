#pragma once
#include <sol/sol.hpp>
#include <vector>
#include <string>
#include <random>
#include <filesystem>
#include "types.hpp"
#include "registry.hpp" // For Stats/Position if needed by Lua registration

class ScriptEngine {
public:
    sol::state lua;
    std::vector<std::string> monster_templates;
    std::vector<std::string> class_templates;
    std::vector<std::string> item_templates;

    ScriptEngine();
    void init_lua();
    void discover_assets();
    bool load_script(const std::string& path);
    std::string pick_from_weights(sol::table weights, std::mt19937& gen);
};