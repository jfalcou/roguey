cmake_minimum_required(VERSION 3.16)
project(ModernRogue CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Curses REQUIRED)
find_package(Lua REQUIRED)


set(SOURCES
    src/main.cpp
    src/dungeon.cpp
    src/systems.cpp
    src/registry.cpp
    src/game.cpp
    src/script_engine.cpp
)

add_executable(rogue_game ${SOURCES})
target_link_libraries(rogue_game PRIVATE ${CURSES_LIBRARIES} ${LUA_LIBRARIES} pthread)
target_include_directories(rogue_game PRIVATE ${CURSES_INCLUDE_DIR} ${LUA_INCLUDE_DIR} include)
include_directories(include)