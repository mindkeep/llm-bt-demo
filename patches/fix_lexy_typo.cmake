# Fix two issues in BehaviorTree.CPP 4.6.2 bundled lexy library.
# When called via PATCH_COMMAND the working directory is the source root.

# Fix 1: operator< typo (_colum_nr should be _column_nr)
set(LEXY_FILE "3rdparty/lexy/include/lexy/input_location.hpp")

if(NOT EXISTS "${LEXY_FILE}")
    message(FATAL_ERROR "lexy file not found: ${LEXY_FILE}")
endif()

file(READ "${LEXY_FILE}" CONTENT)
string(REPLACE "return lhs._column_nr < rhs._colum_nr;"
               "return lhs._column_nr < rhs._column_nr;"
               CONTENT "${CONTENT}")
file(WRITE "${LEXY_FILE}" "${CONTENT}")
message(STATUS "Applied lexy typo fix to ${LEXY_FILE}")

# Fix 2: cmake_minimum_required(VERSION 3.8) warns on cmake 4.x (< 3.10 threshold)
set(LEXY_CMAKE "3rdparty/lexy/CMakeLists.txt")

if(NOT EXISTS "${LEXY_CMAKE}")
    message(FATAL_ERROR "lexy CMakeLists.txt not found: ${LEXY_CMAKE}")
endif()

file(READ "${LEXY_CMAKE}" CONTENT)
string(REPLACE "cmake_minimum_required(VERSION 3.8)"
               "cmake_minimum_required(VERSION 3.10)"
               CONTENT "${CONTENT}")
file(WRITE "${LEXY_CMAKE}" "${CONTENT}")
message(STATUS "Applied cmake_minimum_required fix to ${LEXY_CMAKE}")
