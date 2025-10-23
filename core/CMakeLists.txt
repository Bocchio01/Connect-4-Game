# ---------------------------------------------------------
# Library definition
# ---------------------------------------------------------
add_library(connectx_core STATIC
    src/board.cpp
    src/game/rules.cpp
    src/game/state.cpp
    src/game/engine.cpp
)

add_library(Connectx::core ALIAS connectx_core)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connectx_core
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connectx_core PUBLIC cxx_std_17)
set_target_properties(connectx_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_EXTENSIONS OFF
)

# ---------------------------------------------------------
# Optional logging support (spdlog is fetched in root)
# ---------------------------------------------------------
if(ENABLE_LOGGING)
    target_link_libraries(connectx_core PRIVATE spdlog::spdlog)
    target_compile_definitions(connectx_core PRIVATE ENABLE_LOGGING)
endif()