cmake_minimum_required(VERSION 3.20)
project(Goulag)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

include_directories(src)
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(Goulag ${SOURCES}
        src/core/Config.h
        src/views/MapView.cpp
        src/views/MapView.h)
target_include_directories(Goulag PRIVATE src)

target_link_libraries(Goulag PRIVATE SFML::Graphics SFML::Window SFML::System)

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
