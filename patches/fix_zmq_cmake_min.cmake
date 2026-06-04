# Fix cmake_minimum_required in libzmq v4.3.5 for cmake 4.x compatibility.
# cmake 4.x warns about cmake_minimum_required < 3.10.
# When called via PATCH_COMMAND the working directory is the source root.
set(CMAKELISTS "CMakeLists.txt")

if(NOT EXISTS "${CMAKELISTS}")
    message(FATAL_ERROR "libzmq CMakeLists.txt not found")
endif()

file(READ "${CMAKELISTS}" CONTENT)
string(REPLACE
"if(\${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  cmake_minimum_required(VERSION 3.0.2)
else()
  cmake_minimum_required(VERSION 2.8.12)
endif()"
"cmake_minimum_required(VERSION 3.10)"
CONTENT "${CONTENT}")
file(WRITE "${CMAKELISTS}" "${CONTENT}")
message(STATUS "Applied cmake_minimum_required fix to libzmq CMakeLists.txt")
