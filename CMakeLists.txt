cmake_minimum_required(VERSION 3.20)
project(IronStrike1981 CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0135 NEW)

include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(IronStrike1981 
    src/main.cpp
    src/game/Game.cpp
    src/game/Player.cpp
    src/game/Tank.cpp
    src/game/ExplosionManager.cpp
)

target_include_directories(IronStrike1981 PRIVATE src)
target_link_libraries(IronStrike1981 PRIVATE raylib)
