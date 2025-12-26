#pragma once
#include "registry.hpp"
#include "dungeon.hpp"
#include <sol/sol.hpp>
#include <deque>
#include <string>

struct LogEntry {
    std::string text;
    ColorPair color;
};

class MessageLog {
    std::deque<LogEntry> messages;
    const size_t max_messages = 7;
public:
    void add(std::string msg, ColorPair color = ColorPair::Default);
    void draw(int start_y) const;
};

namespace Systems {
    EntityID get_entity_at(const Registry& reg, int x, int y);
    void check_level_up(Registry& reg, MessageLog& log, sol::state& lua);
    void attack(Registry& reg, EntityID a_id, EntityID d_id, MessageLog& log, sol::state& lua);
    void cast_fireball(Registry& reg, Dungeon& map, int dx, int dy, MessageLog& log);
    void move_monsters(Registry& reg, const Dungeon& map, MessageLog& log, sol::state& lua);
}