#include "renderer.hpp"
#include <ncurses.h>
#include <locale.h> // Required for UTF-8

Renderer::Renderer() {
    setlocale(LC_ALL, ""); // ENABLE UTF-8 SUPPORT
    initscr();
    start_color();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
}

Renderer::~Renderer() {
    endwin();
}

void Renderer::init_colors(ScriptEngine& scripts) {
    sol::table color_table = scripts.lua["game_colors"];
    for (size_t i = 1; i <= color_table.size(); ++i) {
        sol::table entry = color_table[i];
        int id = static_cast<int>(i);
        init_pair(id, entry[1], entry[2]);
        scripts.lua[entry[3]] = id;
    }
}

void Renderer::clear_screen() {
    clear();
}

void Renderer::refresh_screen() {
    refresh();
}

void Renderer::show_error(const std::string& msg) {
    clear();
    attron(A_BOLD);
    mvprintw(10, 10, "CRITICAL ERROR:");
    attroff(A_BOLD);
    mvprintw(12, 10, "%s", msg.c_str());
    mvprintw(14, 10, "[ Press any key to exit ]");
    refresh();
    getch();
}

void Renderer::draw_borders(int w, int h) {
    attron(COLOR_PAIR(0) | A_BOLD);

    // 1. Draw Corners
    mvaddch(0, 0, ACS_ULCORNER);         // ┌
    mvaddch(0, w + 1, ACS_URCORNER);     // ┐
    mvaddch(h + 1, 0, ACS_LLCORNER);     // └
    mvaddch(h + 1, w + 1, ACS_LRCORNER); // ┘

    // 2. Draw Horizontal Lines
    for (int x = 1; x <= w; ++x) {
        mvaddch(0, x, ACS_HLINE);        // ─ Top
        mvaddch(h + 1, x, ACS_HLINE);    // ─ Bottom
        mvaddch(h - 4, x, ACS_HLINE);    // ─ Separator (Above Log)
    }

    // 3. Draw Vertical Lines
    for (int y = 1; y <= h; ++y) {
        mvaddch(y, 0, ACS_VLINE);        // │ Left
        mvaddch(y, w + 1, ACS_VLINE);    // │ Right
    }

    // 4. Draw Connectors for the Separator
    mvaddch(h - 4, 0, ACS_LTEE);         // ├
    mvaddch(h - 4, w + 1, ACS_RTEE);     // ┤

    attroff(COLOR_PAIR(0) | A_BOLD);
}

void Renderer::draw_dungeon(const Dungeon& map, const Registry& reg, const MessageLog& log,
                            int player_id, int depth, int wall_color, int floor_color) {

    // --- 1. DRAW BORDERS ---
    // Map height (20) + Stats (1) + Separator (1) + Log (5) = ~27 lines
    // We define the total UI height here.
    int ui_width = map.width;
    int ui_height = map.height + 7; // Adjust based on your log size
    draw_borders(ui_width, ui_height);

    // --- 2. DRAW MAP (Shifted by 1,1) ---
    int wall_pair = wall_color;
    int floor_pair = floor_color;

    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            // NOTE: mvaddch(y + 1, x + 1, ...) shifts everything inside the border
            if (map.visible_tiles.count({x, y})) {
                attron(COLOR_PAIR(map.grid[y][x] == '#' ? wall_pair : floor_pair));
                mvaddch(y + 1, x + 1, map.grid[y][x]);
                attroff(COLOR_PAIR(map.grid[y][x] == '#' ? wall_pair : floor_pair));
            } else if (map.explored[y][x]) {
                attron(COLOR_PAIR(8));
                mvaddch(y + 1, x + 1, map.grid[y][x]);
                attroff(COLOR_PAIR(8));
            }
        }
    }

    // --- 3. DRAW ENTITIES (Shifted by 1,1) ---
    for (auto const& [id, r] : reg.renderables) {
        Position p = reg.positions.at(id);
        if (id == player_id || map.visible_tiles.count(p)) {
            attron(COLOR_PAIR((short)r.color));
            mvaddch(p.y + 1, p.x + 1, r.glyph);
            attroff(COLOR_PAIR((short)r.color));
        }
    }

    // --- 4. DRAW STATS (Shifted to y=22, below map) ---
    if (reg.stats.contains(player_id)) {
        auto s = reg.stats.at(player_id);
        mvprintw(map.height + 2, 2, "HP: %d/%d | MP: %d/%d | Depth: %d | [I]nv [C]har",
                 s.hp, s.max_hp, s.mana, s.max_mana, depth);
    }

    draw_log(log, map.height + 4, ui_height, ui_width);
}
// NEW: Moved from systems.cpp
void Renderer::draw_log(const MessageLog& log, int start_y, int max_row, int max_col) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int limit_y = (max_row < max_y) ? max_row : max_y;
    int visible_lines = limit_y - start_y + 1;

    if (visible_lines <= 0) return;

    int msg_count = (int)log.messages.size();
    int start_index = 0;
    if (msg_count > visible_lines) {
        start_index = msg_count - visible_lines;
    }

    int draw_y = start_y;
    for (int i = start_index; i < msg_count; ++i) {
        if (draw_y > limit_y) break;

        wmove(stdscr, draw_y, 2);
        int available_width = max_col - 2;
        std::string txt = log.messages[i].text;

        if ((int)txt.length() > available_width - 2) {
            txt = txt.substr(0, available_width - 2);
        }

        attron(COLOR_PAIR(static_cast<short>(log.messages[i].color)));
        printw("> %s", txt.c_str());
        attroff(COLOR_PAIR(static_cast<short>(log.messages[i].color)));

        int cx, cy;
        getyx(stdscr, cy, cx);
        while (cx <= max_col) {
            addch(' ');
            cx++;
        }
        draw_y++;
    }
}

// NEW: Animation Helper
void Renderer::animate_projectile(int x, int y, char glyph, ColorPair color) {
    // Note: x, y are logic coordinates. We need to shift +1,+1 for border.
    attron(COLOR_PAIR(static_cast<short>(color)) | A_BOLD);
    mvaddch(y + 1, x + 1, glyph);
    attroff(COLOR_PAIR(static_cast<short>(color)) | A_BOLD);
    refresh();
    napms(50); // Pause for effect
}

void Renderer::draw_inventory(const std::vector<ItemTag>& inventory) {
    // Inventory uses its own full screen layout, so we don't border this for now
    // or you could add draw_borders(80, 24) here too.
    mvprintw(2, 10, "--- INVENTORY ---");
    if (inventory.empty()) mvprintw(5, 12, "(Empty)");
    else {
        for (size_t i = 0; i < inventory.size(); ++i)
            mvprintw(5 + i, 12, "%zu. %s", i + 1, inventory[i].name.c_str());
    }
    mvprintw(18, 10, "[1-9] Use | [ESC] Exit");
}

void Renderer::draw_stats(const Registry& reg, int player_id, std::string player_name) {
    if (!reg.stats.contains(player_id)) return;
    auto s = reg.stats.at(player_id);
    mvprintw(2, 10, "--- STATS ---");
    mvprintw(5, 12, "Name:   %s", player_name.c_str());
    mvprintw(6, 12, "Level:  %d", s.level);
    mvprintw(7, 12, "Damage: %d", s.damage);
    mvprintw(18, 10, "[ESC] Exit");
}