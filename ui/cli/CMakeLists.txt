# ---------------------------------------------------------
# CLI client executable
# ---------------------------------------------------------
add_executable(connect4_cli
    src/interface.cpp
    app/main.cpp
)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connect4_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# Link dependencies
# ---------------------------------------------------------
target_link_libraries(connect4_cli PRIVATE
    Connect4::client
)

# Note: Connect4::client already includes Connect4::core, Connect4::protocol, and sockpp

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connect4_cli PUBLIC cxx_std_17)
set_target_properties(connect4_cli PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "connect4-cli"
)

if(MSVC)
    target_compile_options(connect4_cli PRIVATE /W4)
else()
    target_compile_options(connect4_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------
# Optional logging support (inherits option from root)
# ---------------------------------------------------------
if(ENABLE_LOGGING)
    target_link_libraries(connect4_cli PRIVATE spdlog::spdlog)
    target_compile_definitions(connect4_cli PRIVATE ENABLE_LOGGING)
endif()

# ---------------------------------------------------------
# Installation (optional, controlled by parent)
# ---------------------------------------------------------
if(INSTALL_CLI)
    include(GNUInstallDirs)

    install(TARGETS connect4_cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()