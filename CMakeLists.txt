cmake_minimum_required(VERSION 3.24)
project(fighting_game LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(fighting_game
  src/main.cpp
  src/input.cpp
  src/fighter.cpp
  src/combat.cpp
  src/stage.cpp
  src/game.cpp
  src/projectile.cpp
  src/selftest.cpp
  src/training.cpp
  src/sprites.cpp
)
target_link_libraries(fighting_game PRIVATE raylib)
