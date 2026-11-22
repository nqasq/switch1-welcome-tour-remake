# PC build (Windows / Linux / macOS). The Switch .nro uses the Makefile instead.
cmake_minimum_required(VERSION 3.16)
project(switch1_welcome_tour LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(TOUR_BUILD_GAME "Build the SDL2/OpenGL demo" ON)
option(TOUR_BUILD_TESTS "Build the physics tests" ON)

# Physics + demo scene: plain C++, no SDL, shared by the game and the tests.
add_library(tour_core STATIC
  src/physics/world.cpp
  src/demo.cpp)
target_include_directories(tour_core PUBLIC src)
if(MSVC)
  target_compile_options(tour_core PUBLIC /W4)
else()
  target_compile_options(tour_core PUBLIC -Wall -Wextra)
endif()

if(TOUR_BUILD_GAME)
  find_package(SDL2 REQUIRED)
  add_executable(switch1-welcome-tour
    src/main.cpp
    src/render/gl.cpp
    src/render/renderer.cpp
    src/input/input.cpp
    src/platform/platform.cpp)
  target_link_libraries(switch1-welcome-tour PRIVATE tour_core)
  if(TARGET SDL2::SDL2main)
    target_link_libraries(switch1-welcome-tour PRIVATE SDL2::SDL2main)
  endif()
  if(TARGET SDL2::SDL2)
    target_link_libraries(switch1-welcome-tour PRIVATE SDL2::SDL2)
  else()
    target_include_directories(switch1-welcome-tour PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(switch1-welcome-tour PRIVATE ${SDL2_LIBRARIES})
  endif()
  # OpenGL functions are loaded at runtime through SDL, so no GL link is needed.
endif()

if(TOUR_BUILD_TESTS)
  enable_testing()
  add_executable(physics_test tests/physics_test.cpp)
  target_link_libraries(physics_test PRIVATE tour_core)
  add_test(NAME physics COMMAND physics_test)
endif()
