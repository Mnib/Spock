cmake_minimum_required(VERSION 3.30)
project(spock VERSION 1.0)
set(LIBRARY_NAME spock)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DNDEBUG)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra")
file(GLOB_RECURSE SOURCE_LIST src/*.cc)

# Dependencies check
find_package(fmt REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)

# ImGUI
file(GLOB IMGUI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui/*.cpp")
add_library(
    imgui STATIC
    ${IMGUI_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui/backends/imgui_impl_vulkan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui/backends/imgui_impl_glfw.cpp"
)

add_library("${LIBRARY_NAME}" STATIC "${SOURCE_LIST}")
target_link_libraries("${LIBRARY_NAME}" PUBLIC glfw Vulkan::Vulkan)
target_link_libraries("${LIBRARY_NAME}" PRIVATE imgui fmt::fmt)
target_include_directories(
    "${LIBRARY_NAME}" PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/third-party")
include_directories(SYSTEM "${CMAKE_CURRENT_SOURCE_DIR}/third-party/imgui")
