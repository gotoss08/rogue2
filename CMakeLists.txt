cmake_minimum_required(VERSION 3.20)
project(rogue VERSION 0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-ggdb -Wall -Wextra -Werror -pedantic)

file(GLOB SOURCES src/*.[c|cpp])

add_executable(rogue)
target_sources(rogue PUBLIC ${SOURCES})
set_property(TARGET rogue PROPERTY C_STANDARD 11)

IF (WIN32)
    set(RAYLIB_DIR c:/code/_libs/raylib-5.0_win64_mingw-w64)
    target_include_directories(rogue PUBLIC ${RAYLIB_DIR}/include)
    target_link_directories(rogue PUBLIC ${RAYLIB_DIR}/lib)
    target_link_libraries(rogue raylib gdi32 winmm)
ELSE()
    find_package(raylib REQUIRED)
    target_link_libraries(rogue raylib)
ENDIF()

