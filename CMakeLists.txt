cmake_minimum_required(VERSION 3.20)
project(Goulag LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/SFML/lib/cmake/SFML" CACHE PATH "SFML CMake directory")
set(SFML_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/SFML/bin" CACHE PATH "SFML DLL folder")

find_package(SFML 2.5 COMPONENTS graphics window system network REQUIRED)

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS
        "${SRC}/models/*.cpp"
        "${SRC}/network/*.cpp"
        "${SRC}/utils/*.cpp"
)

# Client
file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
        "${SRC}/views/*.cpp"
        "${SRC}/controllers/client/*.cpp"
)
list(APPEND CLIENT_SOURCES "${SRC}/core/Game.cpp")

# Server
file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
        "${SRC}/controllers/server/*.cpp"
)
list(APPEND SERVER_SOURCES "${SRC}/core/GameServer.cpp")

# ---------- Lib shared ----------
add_library(GoulagShared STATIC ${SHARED_SOURCES})
target_include_directories(GoulagShared PUBLIC ${SRC})
target_link_libraries(GoulagShared PUBLIC sfml-system sfml-network)

# ---------- Client ----------
add_executable(GoulagClient ${SRC}/main.cpp ${CLIENT_SOURCES})
target_link_libraries(GoulagClient PRIVATE GoulagShared sfml-graphics sfml-window)

# ---------- Server ----------
add_executable(GoulagServer ${SRC}/main_server.cpp ${SERVER_SOURCES})
target_link_libraries(GoulagServer PRIVATE GoulagShared)

# ---------- Post-build ----------
function(copy_sfml_dlls TARGET_NAME)
    if(EXISTS "${SFML_BIN_DIR}")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${SFML_BIN_DIR}" $<TARGET_FILE_DIR:${TARGET_NAME}>
        )
    endif()
endfunction()

copy_sfml_dlls(GoulagClient)
copy_sfml_dlls(GoulagServer)

# Assets copy for client
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET GoulagClient POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:GoulagClient>/assets"
    )
endif()