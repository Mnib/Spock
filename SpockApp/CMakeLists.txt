cmake_minimum_required(VERSION 3.30)
project(spock-app VERSION 1.0)
set(EXECUTABLE_NAME spock-app)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra")
file(GLOB_RECURSE SOURCE_LIST src/*.cc)

find_program(result glslc REQUIRED DOC "Shader compiler")
find_package(glm REQUIRED)

# Compile shaders
message("Compiling shaders...")
file(GLOB_RECURSE VERT_SHADER_LIST "${CMAKE_CURRENT_LIST_DIR}/resources/shaders/*.vert.glsl")
foreach(SHADER_PATH IN LISTS VERT_SHADER_LIST)
    get_filename_component(OUT_NAME "${SHADER_PATH}" NAME_WLE)
    message("${SHADER_PATH}")
    message("${OUT_NAME}")
    execute_process(
        COMMAND glslc -fshader-stage=vert "${SHADER_PATH}" -o "${OUT_NAME}.spv"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/resources/shaders"
        COMMAND_ERROR_IS_FATAL ANY
    )
    message("[OK] ${SHADER_PATH}.spv")
endforeach()

file(GLOB_RECURSE FRAG_SHADER_LIST "${CMAKE_CURRENT_LIST_DIR}/resources/shaders/*.frag.glsl")
foreach(SHADER_PATH IN LISTS FRAG_SHADER_LIST)
    get_filename_component(OUT_NAME "${SHADER_PATH}" NAME_WLE)
    execute_process(
        COMMAND glslc -fshader-stage=frag "${SHADER_PATH}" -o "${OUT_NAME}.spv"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/resources/shaders"
        COMMAND_ERROR_IS_FATAL ANY
    )
    message("[OK] ${SHADER_PATH}")
endforeach()

add_executable("${EXECUTABLE_NAME}" "${SOURCE_LIST}")
target_link_libraries("${EXECUTABLE_NAME}" spock)

include_directories("${CMAKE_CURRENT_LIST_DIR}/include")
include_directories(SYSTEM "${CMAKE_CURRENT_LIST_DIR}/third-party")
