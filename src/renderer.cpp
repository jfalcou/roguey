#include "renderer.hpp"
#include <algorithm>
#include <filesystem>
#include <locale.h>
#include <ncurses.h>

namespace fs = std::filesystem;

Renderer::Renderer()
{
  setlocale(LC_ALL, "");
  initscr();
  start_color();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  getmaxyx(stdscr, screen_height, screen_width);
}

Renderer::~Renderer()
{
  endwin();
}

void Renderer::set_window_size(int w, int h)
{
  screen_width = w;
  screen_height = h;
  // Try to resize the physical terminal if possible, though often ignored by emulators
  resize_term(h, w);
}

void Renderer::init_colors(ScriptEngine& scripts)
{
  sol::table color_table = scripts.lua["game_colors"];
  for (size_t i = 1; i <= color_table.size(); ++i)
  {
    sol::table entry = color_table[i];
    int id = static_cast<int>(i);
    init_pair(id, entry[1], entry[2]);
    scripts.lua[entry[3]] = id;
  }
}

void Renderer::clear_screen()
{
  clear();
}

void Renderer::refresh_screen()
{
  refresh();
}

void Renderer::show_error(std::string const& msg)
{
  clear();
  attron(A_BOLD);
  mvprintw(10, 10, "CRITICAL ERROR:");
  attroff(A_BOLD);
  mvprintw(12, 10, "%s", msg.c_str());
  mvprintw(14, 10, "[ Press any key to exit ]");
  refresh();
  getch();
}

int Renderer::clamp(int val, int min, int max)
{
  if (val < min) return min;
  if (val > max) return max;
  return val;
}

void Renderer::draw_borders(int sx, int sy, int w, int h, int separator_y)
{
  attron(COLOR_PAIR(0) | A_BOLD);

  // Corners
  mvaddch(sy, sx, ACS_ULCORNER);
  mvaddch(sy, sx + w + 1, ACS_URCORNER);
  mvaddch(sy + h + 1, sx, ACS_LLCORNER);
  mvaddch(sy + h + 1, sx + w + 1, ACS_LRCORNER);

  // Horizontals
  for (int x = 1; x <= w; ++x)
  {
    mvaddch(sy, sx + x, ACS_HLINE);
    mvaddch(sy + h + 1, sx + x, ACS_HLINE);
  }

  // Verticals
  for (int y = 1; y <= h; ++y)
  {
    mvaddch(sy + y, sx, ACS_VLINE);
    mvaddch(sy + y, sx + w + 1, ACS_VLINE);
  }

  // Separator
  if (separator_y > 0 && separator_y < h)
  {
    int real_sep_y = sy + separator_y;
    mvaddch(real_sep_y, sx, ACS_LTEE);
    mvaddch(real_sep_y, sx + w + 1, ACS_RTEE);
    for (int x = 1; x <= w; ++x) { mvaddch(real_sep_y, sx + x, ACS_HLINE); }
  }

  attroff(COLOR_PAIR(0) | A_BOLD);
}

void Renderer::draw_log(MessageLog const& log, int start_y, int max_row, int max_col)
{
  int limit_y = max_row;
  int visible_lines = limit_y - start_y + 1;

  if (visible_lines <= 0) return;

  int msg_count = (int)log.messages.size();
  int start_index = 0;
  if (msg_count > visible_lines) { start_index = msg_count - visible_lines; }

  int draw_y = start_y;
  for (int i = start_index; i < msg_count; ++i)
  {
    if (draw_y > limit_y) break;
    wmove(stdscr, draw_y, 2);

    int available_width = max_col - 2;

    std::string txt = log.messages[i].text;
    if ((int)txt.length() > available_width) { txt = txt.substr(0, available_width); }

    attron(COLOR_PAIR(static_cast<short>(log.messages[i].color)));
    printw("> %s", txt.c_str());
    attroff(COLOR_PAIR(static_cast<short>(log.messages[i].color)));

    int cx, cy;
    getyx(stdscr, cy, cx);
    while (cx < max_col)
    {
      addch(' ');
      cx++;
    }

    draw_y++;
  }
}

void Renderer::draw_dungeon(Dungeon const& map,
                            Registry const& reg,
                            MessageLog const& log,
                            int player_id,
                            int depth,
                            int wall_color,
                            int floor_color)
{
  int max_y = screen_height;
  int max_x = screen_width;

  int log_height = 6;
  int stats_lines = 1;

  int ui_overhead = 2 + log_height + 1 + stats_lines;

  int max_map_h = max_y - ui_overhead;
  int max_map_w = max_x - 2;

  int view_w = max_map_w;
  int view_h = max_map_h;

  int frame_h = view_h + 1 + 1 + log_height;

  // Clamp frame to configured screen height if needed
  if (frame_h > max_y - 2) frame_h = max_y - 2;

  Position p = {0, 0};
  if (reg.positions.count(player_id)) p = reg.positions.at(player_id);

  int cam_x, cam_y;

  if (map.width < view_w) { cam_x = -(view_w - map.width) / 2; }
  else
  {
    cam_x = p.x - (view_w / 2);
    cam_x = clamp(cam_x, 0, map.width - view_w);
  }

  if (map.height < view_h) { cam_y = -(view_h - map.height) / 2; }
  else
  {
    cam_y = p.y - (view_h / 2);
    cam_y = clamp(cam_y, 0, map.height - view_h);
  }

  // Draw full size border
  draw_borders(0, 0, view_w, frame_h, view_h + 2);

  int wall_pair = wall_color;
  int floor_pair = floor_color;

  for (int vy = 0; vy < view_h; ++vy)
  {
    for (int vx = 0; vx < view_w; ++vx)
    {
      int wx = cam_x + vx;
      int wy = cam_y + vy;

      if (wx < 0 || wx >= map.width || wy < 0 || wy >= map.height) continue;

      if (map.visible_tiles.count({wx, wy}))
      {
        attron(COLOR_PAIR(map.grid[wy][wx] == '#' ? wall_pair : floor_pair));
        mvaddch(vy + 1, vx + 1, map.grid[wy][wx]);
        attroff(COLOR_PAIR(map.grid[wy][wx] == '#' ? wall_pair : floor_pair));
      }
      else if (map.explored[wy][wx])
      {
        attron(COLOR_PAIR(8));
        mvaddch(vy + 1, vx + 1, map.grid[wy][wx]);
        attroff(COLOR_PAIR(8));
      }
    }
  }

  for (auto const& [id, r] : reg.renderables)
  {
    if (!reg.positions.count(id)) continue;
    Position pos = reg.positions.at(id);

    if (pos.x >= cam_x && pos.x < cam_x + view_w && pos.y >= cam_y && pos.y < cam_y + view_h)
    {

      if (id == player_id || map.visible_tiles.count(pos))
      {
        attron(COLOR_PAIR((short)r.color));
        mvaddch((pos.y - cam_y) + 1, (pos.x - cam_x) + 1, r.glyph);
        attroff(COLOR_PAIR((short)r.color));
      }
    }
  }

  if (reg.stats.contains(player_id))
  {
    auto s = reg.stats.at(player_id);
    mvprintw(view_h + 1, 2, "HP: %d/%d | MP: %d/%d | Depth: %d", s.hp, s.max_hp, s.mana, s.max_mana, depth);
  }

  draw_log(log, view_h + 3, view_h + 2 + log_height, view_w + 1);
}

void Renderer::animate_projectile(int x, int y, char glyph, ColorPair color)
{
  attron(COLOR_PAIR(static_cast<short>(color)) | A_BOLD);
  mvaddch(y + 1, x + 1, glyph);
  attroff(COLOR_PAIR(static_cast<short>(color)) | A_BOLD);
  refresh();
  napms(50);
}

void Renderer::draw_inventory(std::vector<ItemTag> const& inventory, MessageLog const& log)
{
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  // Always draw max size border
  draw_borders(0, 0, ui_width, ui_height, separator_y);

  attron(A_BOLD);
  mvprintw(2, 4, "--- INVENTORY ---");
  attroff(A_BOLD);

  // Content starts below header, but we want it reasonably placed.
  // Standard list from top-left of the content area
  if (inventory.empty()) { mvprintw(4, 6, "(Empty)"); }
  else
  {
    for (size_t i = 0; i < inventory.size(); ++i)
    {
      if (4 + i >= (size_t)separator_y - 1) break;
      mvprintw(4 + i, 6, "[%zu] %s", i + 1, inventory[i].name.c_str());
    }
  }

  mvprintw(separator_y - 1, 2, "[1-9] Use Item | [I/ESC] Close");
  draw_log(log, separator_y + 1, ui_height, ui_width + 1);
}

void Renderer::draw_stats(Registry const& reg, int player_id, std::string player_name, MessageLog const& log)
{
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  // Always draw max size border
  draw_borders(0, 0, ui_width, ui_height, separator_y);

  if (!reg.stats.contains(player_id)) return;
  auto s = reg.stats.at(player_id);

  attron(A_BOLD);
  mvprintw(2, 4, "--- CHARACTER SHEET ---");
  attroff(A_BOLD);

  mvprintw(4, 6, "Name:   %s", player_name.c_str());
  mvprintw(5, 6, "Level:  %d", s.level);
  mvprintw(6, 6, "XP:     %d", s.xp);
  mvprintw(8, 6, "HP:     %d / %d", s.hp, s.max_hp);
  mvprintw(9, 6, "Mana:   %d / %d", s.mana, s.max_mana);
  mvprintw(10, 6, "Damage: %d", s.damage);
  mvprintw(11, 6, "FOV:    %d", s.fov_range);

  mvprintw(separator_y - 1, 2, "[C/ESC] Close");
  draw_log(log, separator_y + 1, ui_height, ui_width + 1);
}

void Renderer::draw_character_creation_header(MessageLog const& log)
{
  clear();
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  // Always draw max size border
  draw_borders(0, 0, ui_width, ui_height, separator_y);

  // Center content vertically in the upper section
  int upper_center_y = separator_y / 2;

  std::string title = "--- CHARACTER CREATION ---";
  std::string prompt = "Enter your name: ";

  mvprintw(upper_center_y - 2, (screen_width - title.length()) / 2, "%s", title.c_str());
  mvprintw(upper_center_y + 1, (screen_width - 20) / 2, "%s", prompt.c_str()); // Approx centering for prompt

  draw_log(log, separator_y + 1, ui_height, ui_width + 1);

  // Position cursor for input right after prompt
  int prompt_x = (screen_width - 20) / 2;
  move(upper_center_y + 1, prompt_x + prompt.length());
  refresh();
}

void Renderer::draw_class_selection(std::vector<std::string> const& class_paths, int selection, MessageLog const& log)
{
  clear();
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  // Always draw max size border
  draw_borders(0, 0, ui_width, ui_height, separator_y);

  std::string title = "--- SELECT YOUR CLASS ---";
  int content_h = class_paths.size() + 2; // title + spacing + items
  int start_y = (separator_y - content_h) / 2;
  if (start_y < 1) start_y = 1;

  mvprintw(start_y, (screen_width - title.length()) / 2, "%s", title.c_str());

  for (size_t i = 0; i < class_paths.size(); ++i)
  {
    if (start_y + 2 + (int)i >= separator_y) break;

    std::string name = fs::path(class_paths[i]).stem().string();
    std::string display = "[ " + name + " ]";
    int x_pos = (screen_width - display.length()) / 2;

    if ((int)i == selection) attron(A_REVERSE);
    mvprintw(start_y + 2 + i, x_pos, "%s", display.c_str());
    attroff(A_REVERSE);
  }

  draw_log(log, separator_y + 1, ui_height, ui_width + 1);
  refresh();
}

void Renderer::draw_game_over(MessageLog const& log)
{
  clear();
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  draw_borders(0, 0, ui_width, ui_height, separator_y);

  int center_y = separator_y / 2;

  attron(A_BOLD | COLOR_PAIR(static_cast<short>(ColorPair::Orc)));
  std::string title = " !!! YOU DIED !!! ";
  mvprintw(center_y - 2, (screen_width - title.length()) / 2, "%s", title.c_str());
  attroff(A_BOLD | COLOR_PAIR(static_cast<short>(ColorPair::Orc)));

  std::string sub = "Press 'r' to Restart, 'q' to Quit";
  mvprintw(center_y + 1, (screen_width - sub.length()) / 2, "%s", sub.c_str());

  draw_log(log, separator_y + 1, ui_height, ui_width + 1);
  refresh();
}

void Renderer::draw_victory(MessageLog const& log)
{
  clear();
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  draw_borders(0, 0, ui_width, ui_height, separator_y);

  int center_y = separator_y / 2;

  attron(A_BOLD | COLOR_PAIR(static_cast<short>(ColorPair::Gold)));
  std::string title = " !!! VICTORY !!! ";
  mvprintw(center_y - 2, (screen_width - title.length()) / 2, "%s", title.c_str());
  attroff(A_BOLD | COLOR_PAIR(static_cast<short>(ColorPair::Gold)));

  std::string sub = "Press 'c' to Continue, 'q' to Quit";
  mvprintw(center_y + 1, (screen_width - sub.length()) / 2, "%s", sub.c_str());

  draw_log(log, separator_y + 1, ui_height, ui_width + 1);
  refresh();
}
