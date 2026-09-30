cmake_minimum_required(VERSION 3.20)
project(Goulag)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/SFML/lib/cmake/SFML" CACHE PATH "SFML CMake directory")
set(SFML_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/SFML/bin" CACHE PATH "SFML DLL folder")

find_package(SFML 2.5.1 COMPONENTS graphics window system REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(Goulag ${SOURCES})

target_include_directories(Goulag PRIVATE src)
target_link_libraries(Goulag PRIVATE sfml-graphics sfml-window sfml-system)

if(EXISTS "${SFML_BIN_DIR}")
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