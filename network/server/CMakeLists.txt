# ---------------------------------------------------------
# Server library (for potential reuse in tests)
# ---------------------------------------------------------
add_library(connectx_server_lib STATIC
    src/connection.cpp
    src/session_manager.cpp
    src/game_session.cpp
    src/game_server.cpp
)

add_library(Connectx::server_lib ALIAS connectx_server_lib)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connectx_server_lib
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------
# Link dependencies (sockpp and Threads are fetched/found in root)
# ---------------------------------------------------------
target_link_libraries(connectx_server_lib
    PUBLIC
    Connectx::core
    Connectx::protocol
    sockpp
    Threads::Threads
    spdlog::spdlog
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(connectx_server_lib PUBLIC ws2_32)
endif()

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connectx_server_lib PUBLIC cxx_std_17)
set_target_properties(connectx_server_lib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_EXTENSIONS OFF
)

# ---------------------------------------------------------
# Server executable
# ---------------------------------------------------------
add_executable(connectx_server
    app/main.cpp
)

target_link_libraries(connectx_server PRIVATE connectx_server_lib)

set_target_properties(connectx_server PROPERTIES
    OUTPUT_NAME "connectx-server"
)

# ---------------------------------------------------------
# Installation (optional, controlled by parent)
# ---------------------------------------------------------
if(INSTALL_EXECUTABLES)
    include(GNUInstallDirs)

    install(TARGETS connectx_server
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()