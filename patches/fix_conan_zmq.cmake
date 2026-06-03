# Patch conan_build.cmake to use libzmq from FetchContent when available,
# falling back to find_package(ZeroMQ) for system installations.
set(CONAN_FILE "cmake/conan_build.cmake")

if(NOT EXISTS "${CONAN_FILE}")
    message(FATAL_ERROR "conan_build.cmake not found: ${CONAN_FILE}")
endif()

file(READ "${CONAN_FILE}" CONTENT)

# Replace the find_package(ZeroMQ REQUIRED) block with a version that
# accepts libzmq targets already in scope from FetchContent.
string(REPLACE
"if(BTCPP_GROOT_INTERFACE)
    find_package(ZeroMQ REQUIRED)
    list(APPEND BTCPP_EXTRA_LIBRARIES \${ZeroMQ_LIBRARIES})
    list(APPEND BTCPP_EXTRA_INCLUDE_DIRS \${ZeroMQ_INCLUDE_DIRS})
    message(STATUS \"ZeroMQ_LIBRARIES: \${ZeroMQ_LIBRARIES}\")
endif()"
"if(BTCPP_GROOT_INTERFACE)
    if(TARGET libzmq OR TARGET libzmq-static)
        # libzmq already available (e.g., via FetchContent in parent project)
        if(TARGET libzmq)
            list(APPEND BTCPP_EXTRA_LIBRARIES libzmq)
        else()
            list(APPEND BTCPP_EXTRA_LIBRARIES libzmq-static)
        endif()
        if(TARGET libzmq)
            get_target_property(_zmq_inc libzmq INTERFACE_INCLUDE_DIRECTORIES)
        else()
            get_target_property(_zmq_inc libzmq-static INTERFACE_INCLUDE_DIRECTORIES)
        endif()
        if(_zmq_inc)
            list(APPEND BTCPP_EXTRA_INCLUDE_DIRS \${_zmq_inc})
        endif()
        message(STATUS \"ZeroMQ: using FetchContent target\")
    else()
        find_package(ZeroMQ REQUIRED)
        list(APPEND BTCPP_EXTRA_LIBRARIES \${ZeroMQ_LIBRARIES})
        list(APPEND BTCPP_EXTRA_INCLUDE_DIRS \${ZeroMQ_INCLUDE_DIRS})
        message(STATUS \"ZeroMQ_LIBRARIES: \${ZeroMQ_LIBRARIES}\")
    endif()
endif()"
CONTENT "${CONTENT}")

file(WRITE "${CONAN_FILE}" "${CONTENT}")
message(STATUS "Applied ZeroMQ FetchContent patch to ${CONAN_FILE}")
