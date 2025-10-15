# ---------------------------------------------------------
# Library definition
# ---------------------------------------------------------
add_library(connect4_core STATIC
    src/board.cpp
    src/game/rules.cpp
    src/game/state.cpp
    src/game/engine.cpp
)

add_library(Connect4::core ALIAS connect4_core)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connect4_core
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connect4_core PUBLIC cxx_std_17)
set_target_properties(connect4_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_EXTENSIONS OFF
)

# ---------------------------------------------------------
# Optional logging support (spdlog is fetched in root)
# ---------------------------------------------------------
if(ENABLE_LOGGING)
    target_link_libraries(connect4_core PRIVATE spdlog::spdlog)
    target_compile_definitions(connect4_core PRIVATE ENABLE_LOGGING)
endif()