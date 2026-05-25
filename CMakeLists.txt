cmake_minimum_required(VERSION 3.20)

project(PC110EMUPortable C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PC110_BUILD_SDL "Build the optional SDL2 GUI frontend" ON)
option(PC110_BUILD_FIRMWARE_MODEL "Build the clean-room C++ PC110 firmware behavior model" ON)

add_library(pc110core STATIC
    Sources/PC110Core/pc110_core.c
)
target_include_directories(pc110core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/PC110Core/include
)
target_compile_definitions(pc110core PRIVATE
    _CRT_SECURE_NO_WARNINGS
)
if(MSVC)
    target_compile_options(pc110core PRIVATE /O2)
else()
    target_compile_options(pc110core PRIVATE -O3)
endif()

add_library(pc110portable STATIC
    Sources/PC110Portable/pc110_portable_common.c
)
target_include_directories(pc110portable PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/PC110Portable
)
target_link_libraries(pc110portable PUBLIC pc110core)

add_executable(pc110emu-headless
    Sources/PC110Portable/pc110_headless.c
)
target_link_libraries(pc110emu-headless PRIVATE pc110portable)

if(PC110_BUILD_FIRMWARE_MODEL)
    add_library(pc110firmware STATIC
        Sources/PC110Firmware/PC110FirmwareModel.cpp
    )
    target_include_directories(pc110firmware PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Sources/PC110Firmware/include
    )

    add_executable(pc110firmware-smoke
        Sources/PC110Firmware/pc110_firmware_smoke.cpp
    )
    target_link_libraries(pc110firmware-smoke PRIVATE pc110firmware)
endif()

if(PC110_BUILD_SDL)
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        add_executable(pc110emu-sdl
            Sources/PC110Portable/pc110_sdl.c
        )
        target_link_libraries(pc110emu-sdl PRIVATE pc110portable)
        if(TARGET SDL2::SDL2)
            target_link_libraries(pc110emu-sdl PRIVATE SDL2::SDL2)
        else()
            target_include_directories(pc110emu-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
            target_link_libraries(pc110emu-sdl PRIVATE ${SDL2_LIBRARIES})
        endif()
        if(TARGET SDL2::SDL2main)
            target_link_libraries(pc110emu-sdl PRIVATE SDL2::SDL2main)
        endif()
    else()
        message(STATUS "SDL2 not found; building pc110emu-headless only")
    endif()
endif()
