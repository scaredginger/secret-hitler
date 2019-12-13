# cmake_minimum_required(VERSION <specify CMake version here>)
project(server)

set(CMAKE_CXX_STANDARD 17)

add_executable(server main.cpp game.cpp game.h player.cpp player.h communicationManager.cpp communicationManager.h common.h)