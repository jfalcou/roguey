#pragma once
#include <stdint.h>
#include <vector>
#include <string>

enum class GameState { Dungeon, Inventory, Stats, GameOver, Victory };

enum class ColorPair : short {
    Default = 1, Player, Orc, Wall, Floor, Gold, Spell, Potion, Hidden, HP_Potion, Boss
};

// Added Stairs type
enum class ItemType { Gold, Consumable, Stairs };
using EntityID = uint32_t;

struct Position {
    int x, y;
    bool operator<(const Position& other) const {
        return y < other.y || (y == other.y && x < other.x);
    }
    bool operator==(const Position& other) const { return x == other.x && y == other.y; }
};

struct Stats { int hp, max_hp, mana, max_mana, damage, xp, level, fov_range; };

struct ItemTag {
    ItemType type;
    int value;
    std::string name;
    std::string script;
};

struct Rect {
    int x, y, w, h;
    Position center() const { return {x + w/2, y + h/2}; }
    bool intersects(const Rect& other) const {
        return (x <= other.x + other.w && x + w >= other.x &&
                y <= other.y + other.h && y + h >= other.y);
    }
};