#pragma once
#include "registry.hpp"
#include "dungeon.hpp"
#include <sol/sol.hpp>
#include <deque>
#include <string>

class Renderer;

struct LogEntry {
    std::string text;
    ColorPair color;
};

class MessageLog {
public:
    std::deque<LogEntry> messages;
    const size_t max_messages = 10;

    void add(std::string msg, ColorPair color = ColorPair::Default);
};

namespace Systems {
    EntityID get_entity_at(const Registry& reg, int x, int y);
    void check_level_up(Registry& reg, MessageLog& log, sol::state& lua);
    void attack(Registry& reg, EntityID a_id, EntityID d_id, MessageLog& log, sol::state& lua);

    void cast_fireball(Registry& reg, Dungeon& map, int dx, int dy, MessageLog& log, Renderer& renderer);
    void move_monsters(Registry& reg, const Dungeon& map, MessageLog& log, sol::state& lua);
}