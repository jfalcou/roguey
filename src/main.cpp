//==================================================================================================
/*
  Roguey
  Copyright : Joel FALCOU
  SPDX-License-Identifier: MIT
*/
//==================================================================================================

#include "game.hpp"
#include <atomic>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char* argv[])
{
  bool debug = false;
  std::vector<std::string> args(argv + 1, argv + argc);
  for (auto const& arg : args)
  {
    if (arg == "-d") debug = true;
  }

  struct working_dir_is_exe_dir
  {
    std::filesystem::path const original_working_dir = std::filesystem::current_path();

    explicit working_dir_is_exe_dir(std::string_view exe_path_str)
    {
      // from now on we want all file loading to be relative to this executable's path
      // to achieve this we force the current path to the directory where the executable is located
      namespace fs = std::filesystem;
      auto const exe_path = fs::canonical(exe_path_str);
      auto const exe_dir_path = exe_path.parent_path();
      assert(is_directory(exe_dir_path));
      fs::current_path(exe_dir_path);
    }

    ~working_dir_is_exe_dir() { std::filesystem::current_path(original_working_dir); }
  } change_working_dir [[maybe_unused]]{argv[0]};

  roguey::Game game(debug);

  if (!ftxui::Terminal::ColorSupport())
  {
    std::cout << "WARNING: Your terminal does not support TrueColor." << std::endl;
    std::cout << "Try running with: COLORTERM=truecolor ./rogue_game" << std::endl;
    // You might want to getchar() here to read it before the UI takes over
    char c;
    std::cin >> c;
  }
  auto screen = ftxui::ScreenInteractive::Fullscreen();

  // Background thread to drive animations (approx 20 FPS)
  std::atomic<bool> tick_running = true;
  std::thread ticker([&screen, &tick_running]() {
    while (tick_running)
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(50ms);
      // Post a Special event {0} to act as a "Tick"
      screen.PostEvent(ftxui::Event::Special({0}));
    }
  });

  auto component = ftxui::Renderer([&] { return game.render_ui(); });

  component |= ftxui::CatchEvent([&](ftxui::Event event) {
    bool handled = game.on_event(event);

    // Check if game logic requested a quit
    if (!game.is_running())
    {
      screen.Exit();
      return true;
    }

    // Global interrupt: Ctrl+C
    if (event == ftxui::Event::Character('\x03'))
    {
      screen.Exit();
      return true;
    }

    return handled;
  });

  screen.Loop(component);

  tick_running = false;
  if (ticker.joinable()) ticker.join();

  return 0;
}
