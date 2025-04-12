cmake_minimum_required(VERSION 3.30)
project(Spock VERSION 1.0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

option(BUILD_EXAMPLES "Build examples" "OFF")
option(ENABLE_ASAN "Enable ASAN" "ON")

if(ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()


add_subdirectory(Spock)

if (BUILD_EXAMPLES)
    add_subdirectory(SpockApp)
endif()
