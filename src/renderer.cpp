#include "renderer.hpp"
#include <algorithm>
#include <filesystem>
#include <locale.h>
#include <ncurses.h>
#include <sstream> // Added for stringstream

namespace fs = std::filesystem;

Renderer::Renderer()
{
  setlocale(LC_ALL, "");
  initscr();
  start_color();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  set_escdelay(25);
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
  resize_term(h, w);
}

int Renderer::get_color(std::string const& name, int def) const
{
  if (lua_state) return (*lua_state)[name].get_or(COLOR_PAIR(def));
  return COLOR_PAIR(def);
}

void Renderer::init_colors(ScriptEngine& scripts)
{
  this->lua_state = &scripts.lua;
  sol::table color_table = scripts.lua["game_colors"];
  for (size_t i = 1; i <= color_table.size(); ++i)
  {
    sol::table entry = color_table[i];
    int id = static_cast<int>(i);
    init_pair(id, entry[1], entry[2]);

    bool is_bright = entry[3].get_or(false);
    int final_attr = COLOR_PAIR(id);
    if (is_bright) final_attr |= A_BOLD;

    scripts.lua[entry[4]] = final_attr;
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

void Renderer::draw_borders(int sx, int sy, int w, int h, std::string const& title, int separator_y)
{
  int cid = get_color("ui_border");
  attron(cid);

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

  if (!title.empty())
  {
    std::string label = "[ " + title + " ]";
    if ((int)label.length() <= w)
    {
      int label_x = sx + 1 + (w - (int)label.length()) / 2;
      mvprintw(sy, label_x, "%s", label.c_str());
    }
  }

  attroff(cid);
}

void Renderer::draw_log(MessageLog const& log, int start_y, int max_row, int max_col)
{
  int limit_y = max_row;
  int visible_lines = limit_y - start_y + 1;

  if (visible_lines <= 0) return;

  int msg_count = static_cast<int>(log.messages.size());
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

    auto color_id = get_color(log.messages[i].color);

    attron(color_id);
    printw("> %s", txt.c_str());
    attroff(color_id);

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
                            int floor_color,
                            std::string const& level_name)
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

  // Update stored camera position for other effects (like projectiles)
  last_cam_x = cam_x;
  last_cam_y = cam_y;

  draw_borders(0, 0, view_w, frame_h, level_name, view_h + 2);

  for (int vy = 0; vy < view_h; ++vy)
  {
    for (int vx = 0; vx < view_w; ++vx)
    {
      int wx = cam_x + vx;
      int wy = cam_y + vy;

      if (wx < 0 || wx >= map.width || wy < 0 || wy >= map.height) continue;

      if (map.visible_tiles.count({wx, wy}))
      {
        int col_attr = map.grid[wy][wx] == '#' ? wall_color : floor_color;

        attron(col_attr);
        mvaddch(vy + 1, vx + 1, map.grid[wy][wx]);
        attroff(col_attr);
      }
      else if (map.explored[wy][wx])
      {
        attron(get_color("ui_hidden"));
        mvaddch(vy + 1, vx + 1, map.grid[wy][wx]);
        attroff(get_color("ui_hidden"));
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
        attron(r.color);
        mvaddch((pos.y - cam_y) + 1, (pos.x - cam_x) + 1, r.glyph);
        attroff(r.color);
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

void Renderer::animate_projectile(int x, int y, char glyph, std::string const& color)
{
  // Convert map coordinates to screen coordinates using the last camera position
  int sx = x - last_cam_x + 1;
  int sy = y - last_cam_y + 1;

  auto c = get_color(color);
  attron(c);
  mvaddch(sy, sx, glyph);
  attroff(c);
  refresh();
  napms(50);
}

void Renderer::draw_inventory(std::vector<ItemTag> const& inventory, MessageLog const& log)
{
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  draw_borders(0, 0, ui_width, ui_height, "INVENTORY", separator_y);

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

  draw_borders(0, 0, ui_width, ui_height, "STATS", separator_y);

  if (!reg.stats.contains(player_id)) return;
  auto s = reg.stats.at(player_id);

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

void Renderer::draw_help(MessageLog const& log, std::string const& help_text)
{
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  draw_borders(0, 0, ui_width, ui_height, "HELP", separator_y);

  // Parse and print line by line
  int start_y = 4;
  int start_x = 6;

  std::stringstream ss(help_text);
  std::string line;
  int i = 0;
  while (std::getline(ss, line, '\n'))
  {
    if (start_y + i >= separator_y - 1) break;
    mvprintw(start_y + i, start_x, "%s", line.c_str());
    i++;
  }

  mvprintw(separator_y - 1, 2, "[ESC] Back");
  draw_log(log, separator_y + 1, ui_height, ui_width + 1);
}

void Renderer::draw_character_creation_header(MessageLog const& log)
{
  clear();
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  draw_borders(0, 0, ui_width, ui_height, "CREATE YOUR HERO", separator_y);

  int upper_center_y = separator_y / 2;
  std::string prompt = "Enter your name: ";

  mvprintw(upper_center_y, (screen_width - 20) / 2, "%s", prompt.c_str());

  draw_log(log, separator_y + 1, ui_height, ui_width + 1);

  int prompt_x = (screen_width - 20) / 2;
  move(upper_center_y, prompt_x + prompt.length());
  refresh();
}

void Renderer::draw_class_selection(std::vector<std::string> const& class_paths, int selection, MessageLog const& log)
{
  clear();
  int ui_width = screen_width - 2;
  int ui_height = screen_height - 2;
  int log_height = 6;
  int separator_y = ui_height - log_height - 1;

  draw_borders(0, 0, ui_width, ui_height, "CREATE YOUR HERO", separator_y);

  std::string subtitle = "Select your Class";

  int content_h = class_paths.size() + 2 + 2;
  int start_y = (separator_y - content_h) / 2;
  if (start_y < 1) start_y = 1;

  mvprintw(start_y, (screen_width - subtitle.length()) / 2, "%s", subtitle.c_str());

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

  draw_borders(0, 0, ui_width, ui_height, "GAME OVER", separator_y);

  int center_y = separator_y / 2;

  attron(get_color("ui_failure"));
  std::string title = " !!! YOU DIED !!! ";
  mvprintw(center_y - 2, (screen_width - title.length()) / 2, "%s", title.c_str());
  attroff(get_color("ui_failure"));

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

  draw_borders(0, 0, ui_width, ui_height, "VICTORY", separator_y);

  int center_y = separator_y / 2;

  attron(A_BOLD | COLOR_PAIR(get_color("ui_gold")));
  std::string title = " !!! VICTORY !!! ";
  mvprintw(center_y - 2, (screen_width - title.length()) / 2, "%s", title.c_str());
  attroff(A_BOLD | COLOR_PAIR(get_color("ui_gold")));

  std::string sub = "Press 'c' to Continue, 'q' to Quit";
  mvprintw(center_y + 1, (screen_width - sub.length()) / 2, "%s", sub.c_str());

  draw_log(log, separator_y + 1, ui_height, ui_width + 1);
  refresh();
}
