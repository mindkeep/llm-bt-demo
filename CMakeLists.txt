cmake_minimum_required(VERSION 3.16)
project(llm_bt_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# libzmq — required for BT.CPP Groot2Publisher
# Build as static to avoid shared-library deployment issues.
FetchContent_Declare(
    libzmq
    GIT_REPOSITORY https://github.com/zeromq/libzmq.git
    GIT_TAG        v4.3.5
)
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC ON  CACHE BOOL "" FORCE)
set(WITH_PERF_TOOL OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)
# CMake 4.x rejects cmake_minimum_required < 3.5 — temporarily allow it for libzmq v4.3.5
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_MakeAvailable(libzmq)
unset(CMAKE_POLICY_VERSION_MINIMUM CACHE)

# BehaviorTree.CPP v4
# PATCH_COMMAND applies two fixes:
#   1. lexy typo (operator< uses _colum_nr instead of _column_nr)
#   2. conan_build.cmake ZeroMQ: prefer FetchContent target over find_package
FetchContent_Declare(
    behaviortree_cpp
    GIT_REPOSITORY https://github.com/BehaviorTree/BehaviorTree.CPP.git
    GIT_TAG        4.6.2
    PATCH_COMMAND  ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/patches/fix_lexy_typo.cmake
              COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/patches/fix_conan_zmq.cmake
)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# cpp-httplib (header-only)
FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)

# nlohmann/json (header-only)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

set(FETCHCONTENT_QUIET OFF)

# Disable optional BehaviorTree.CPP features that require extra system deps
set(BTCPP_SQLITE_LOGGING  OFF CACHE BOOL "" FORCE)
set(BTCPP_EXAMPLES        OFF CACHE BOOL "" FORCE)
set(BTCPP_UNIT_TESTS      OFF CACHE BOOL "" FORCE)
set(BTCPP_BUILD_TOOLS     OFF CACHE BOOL "" FORCE)
set(BTCPP_SHARED_LIBS     OFF CACHE BOOL "" FORCE)
# Enable Groot2 publisher (libzmq is provided via FetchContent above)
set(BTCPP_GROOT_INTERFACE ON  CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(behaviortree_cpp googletest cpp_httplib nlohmann_json)

add_subdirectory(libs/world_sim)
add_subdirectory(libs/bt_nodes)
add_subdirectory(libs/llm_client)

add_executable(llm_bt_demo src/main.cpp)
target_link_libraries(llm_bt_demo PRIVATE
    world_sim_lib
    bt_nodes_lib
    llm_client_lib
    behaviortree_cpp
)
target_include_directories(llm_bt_demo PRIVATE src)

enable_testing()
add_subdirectory(tests)
