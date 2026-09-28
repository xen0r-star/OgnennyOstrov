cmake_minimum_required(VERSION 3.20)
project(Goulag)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{SFML_DIR})
    set(SFML_DIR "$ENV{SFML_DIR}")
elseif(DEFINED ENV{SFML_ROOT})
    set(SFML_DIR "$ENV{SFML_ROOT}/lib/cmake/SFML")
endif()

find_package(SFML 2.5.1 COMPONENTS graphics window system REQUIRED)

include_directories(src)
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(Goulag ${SOURCES}
        src/core/Config.h
        src/views/MapView.cpp
        src/views/MapView.h)

target_include_directories(Goulag PRIVATE src)

target_link_libraries(Goulag PRIVATE sfml-graphics sfml-window sfml-system)

if(NOT DEFINED SFML_BIN_DIR AND DEFINED ENV{SFML_BIN_DIR})
    set(SFML_BIN_DIR "$ENV{SFML_BIN_DIR}")
endif()

if(DEFINED SFML_BIN_DIR)
    add_custom_command(TARGET Goulag POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SFML_BIN_DIR}"
            $<TARGET_FILE_DIR:Goulag>
    )
endif()

add_custom_command(TARGET Goulag POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Goulag>/assets"
)