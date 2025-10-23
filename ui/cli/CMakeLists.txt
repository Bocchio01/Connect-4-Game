# ---------------------------------------------------------
# CLI client executable
# ---------------------------------------------------------
add_executable(connectx_cli
    src/visual.cpp
    src/output.cpp
    src/input.cpp
    src/interface.cpp
    app/main.cpp
)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connectx_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ---------------------------------------------------------
# Link dependencies
# ---------------------------------------------------------
target_link_libraries(connectx_cli PRIVATE
    Connectx::client
)

# Note: Connectx::client already includes Connectx::core, Connectx::protocol, and sockpp

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connectx_cli PUBLIC cxx_std_17)
set_target_properties(connectx_cli PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "connectx-cli"
)

# ---------------------------------------------------------
# Optional logging support (spdlog is fetched in root)
# ---------------------------------------------------------
if(ENABLE_LOGGING)
    target_link_libraries(connectx_cli PRIVATE spdlog::spdlog)
    target_compile_definitions(connectx_cli PRIVATE ENABLE_LOGGING)
endif()

# ---------------------------------------------------------
# Installation (optional, controlled by parent)
# ---------------------------------------------------------
if(INSTALL_EXECUTABLES)
    include(GNUInstallDirs)

    install(TARGETS connectx_cli
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()