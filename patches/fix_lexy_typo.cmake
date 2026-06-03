# Fix typo in BehaviorTree.CPP 4.6.2 bundled lexy library:
# operator< uses _colum_nr instead of _column_nr
set(LEXY_FILE
    "${CMAKE_CURRENT_LIST_DIR}/../build/_deps/behaviortree_cpp-src/3rdparty/lexy/include/lexy/input_location.hpp")

# Resolve the relative path using cmake -P invocation context
# When called via PATCH_COMMAND the working directory is the source root
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
