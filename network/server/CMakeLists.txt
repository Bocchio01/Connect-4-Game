# ---------------------------------------------------------
# Fetch sockpp library
# ---------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    sockpp
    GIT_REPOSITORY https://github.com/fpagliughi/sockpp.git
    GIT_TAG v0.8.3
)
FetchContent_MakeAvailable(sockpp)

# Disable warnings for sockpp target
if(TARGET sockpp-objs)
    target_compile_options(sockpp-objs PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/w>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>
    )
    set_target_properties(sockpp-objs PROPERTIES
        CXX_CLANG_TIDY ""
        CXX_CPPCHECK ""
    )
endif()

# ---------------------------------------------------------
# Fetch thread library
# ---------------------------------------------------------
find_package(Threads REQUIRED)
if(NOT Threads_FOUND)
    message(FATAL_ERROR "Threads library not found")
endif()

# ---------------------------------------------------------
# Server library (for potential reuse in tests)
# ---------------------------------------------------------
add_library(connect4_server_lib STATIC
    src/connection.cpp
    src/session_manager.cpp
    src/game_session.cpp
    src/game_server.cpp
)

add_library(Connect4::server_lib ALIAS connect4_server_lib)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connect4_server_lib
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------
# Link dependencies
# ---------------------------------------------------------
target_link_libraries(connect4_server_lib
    PUBLIC
    Connect4::core
    Connect4::protocol
    sockpp
    Threads::Threads
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(connect4_server_lib PUBLIC ws2_32)
endif()

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connect4_server_lib PUBLIC cxx_std_17)
set_target_properties(connect4_server_lib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(connect4_server_lib PRIVATE /W4)
else()
    target_compile_options(connect4_server_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------
# Server executable
# ---------------------------------------------------------
add_executable(connect4_server
    app/main.cpp
)

target_link_libraries(connect4_server PRIVATE connect4_server_lib)

set_target_properties(connect4_server PROPERTIES
    OUTPUT_NAME "connect4-server"
)

# ---------------------------------------------------------
# Optional logging support (inherits option from root)
# ---------------------------------------------------------
if(ENABLE_LOGGING)
    target_link_libraries(connect4_server_lib PRIVATE spdlog::spdlog)
    target_compile_definitions(connect4_server_lib PRIVATE ENABLE_LOGGING)
endif()

# ---------------------------------------------------------
# Installation (optional, controlled by parent)
# ---------------------------------------------------------
if(INSTALL_SERVER)
    include(GNUInstallDirs)

    install(TARGETS connect4_server
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(TARGETS connect4_server_lib
        EXPORT Connect4ServerTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT Connect4ServerTargets
        FILE Connect4ServerTargets.cmake
        NAMESPACE Connect4::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Connect4Server
    )
endif()