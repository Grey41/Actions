cmake_minimum_required(VERSION 3.23)
project(JoBase)

find_package(Python3 ${PYTHON_VERSION} EXACT COMPONENTS Development.Module)
add_subdirectory(lib/sdl lib/sdl/build)
add_subdirectory(lib/mix lib/mix/build)

file(GLOB SOURCES src/*.c lib/libtess2/Source/*.c)
add_library(JoBase MODULE ${SOURCES})

target_include_directories(JoBase PRIVATE include lib/stb lib/libtess2/Include)
target_link_libraries(JoBase PRIVATE SDL3::SDL3 SDL3_mixer::SDL3_mixer Python3::Module)

set_target_properties(JoBase PROPERTIES
    PREFIX ""
    SUFFIX ""
    OUTPUT_NAME ${JOBASE_FILE}
    LIBRARY_OUTPUT_DIRECTORY ${JOBASE_DIR})