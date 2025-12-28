##==================================================================================================
##  Roguey
##  Copyright : Joel FALCOU
##  SPDX-License-Identifier: MIT
##==================================================================================================
cmake_minimum_required(VERSION 3.16)
project(ROGUEY CXX)

set(CMAKE_CXX_STANDARD 20)

##==================================================================================================
## Dependencies
##==================================================================================================
include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v5.0.0
)
FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

find_package(Lua REQUIRED)

##==================================================================================================
## Source code
##==================================================================================================
set(SOURCES
    src/dungeon.cpp
    src/game.cpp
    src/main.cpp
    src/registry.cpp
    src/renderer.cpp
    src/script_engine.cpp
    src/systems.cpp
)

file(GLOB_RECURSE LUA_SCRIPTS "scripts/*.lua")

include_directories(include)
add_executable(rogue_game ${SOURCES} ${LUA_SCRIPTS})

target_link_libraries(rogue_game PRIVATE ftxui::screen ftxui::dom ftxui::component ${LUA_LIBRARIES} pthread)
target_include_directories(rogue_game PRIVATE ${LUA_INCLUDE_DIR} include)

##==================================================================================================
## Copy script to binary
##==================================================================================================
add_custom_target(copy_scripts
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/scripts/
    ${CMAKE_BINARY_DIR}/scripts/
    COMMENT "Checking for updated scripts..."
)

# 2. Add Dependency
add_dependencies(rogue_game copy_scripts)