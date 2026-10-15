cmake_minimum_required(VERSION 3.20)
project(OgnennyOstrov LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(WIN32)
    set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/SFML/lib/cmake/SFML" CACHE PATH "SFML CMake directory")
    set(SFML_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/SFML/bin" CACHE PATH "SFML DLL folder")
endif()

find_package(SFML 2.5 COMPONENTS graphics window system network REQUIRED)

file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS
        "${SRC}/models/*.cpp"
        "${SRC}/network/*.cpp"
        "${SRC}/utils/*.cpp"
)

# Client sources
file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
        "${SRC}/views/*.cpp"
        "${SRC}/controllers/client/*.cpp"
)
list(APPEND CLIENT_SOURCES "${SRC}/core/Game.cpp")

# Server sources
file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
        "${SRC}/controllers/server/*.cpp"
)
list(APPEND SERVER_SOURCES "${SRC}/core/GameServer.cpp")

# ---------- Lib shared ----------
add_library(OgnennyOstrovShared STATIC ${SHARED_SOURCES})
target_include_directories(OgnennyOstrovShared PUBLIC ${SRC})
target_link_libraries(OgnennyOstrovShared PUBLIC sfml-system sfml-network sfml-audio)

# ---------- Client ----------
add_executable(OgnennyOstrov ${SRC}/main.cpp ${CLIENT_SOURCES})
target_link_libraries(OgnennyOstrov PRIVATE OgnennyOstrovShared sfml-graphics sfml-window)

# ---------- Server ----------
add_executable(OgnennyOstrovServer ${SRC}/main_server.cpp ${SERVER_SOURCES})
target_link_libraries(OgnennyOstrovServer PRIVATE OgnennyOstrovShared sfml-system sfml-network)

# ---------- Post-build (Windows uniquement) ----------
if(WIN32)
    function(copy_sfml_dlls TARGET_NAME)
        if(EXISTS "${SFML_BIN_DIR}")
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${SFML_BIN_DIR}" $<TARGET_FILE_DIR:${TARGET_NAME}>
            )
        endif()
    endfunction()

    copy_sfml_dlls(OgnennyOstrov)
    copy_sfml_dlls(OgnennyOstrovServer)

    # Assets copy for client
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
        add_custom_command(TARGET OgnennyOstrov POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:OgnennyOstrov>/assets"
        )
    endif()
endif()