# ---------------------------------------------------------
# Client library
# ---------------------------------------------------------
add_library(connect4_client STATIC
    src/client.cpp
)

add_library(Connect4::client ALIAS connect4_client)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connect4_client
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------
# Link dependencies (sockpp and Threads are fetched/found in root)
# ---------------------------------------------------------
target_link_libraries(connect4_client
    PUBLIC
    Connect4::core
    Connect4::protocol
    sockpp
    Threads::Threads
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(connect4_client PUBLIC ws2_32)
endif()

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connect4_client PUBLIC cxx_std_17)
set_target_properties(connect4_client PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_EXTENSIONS OFF
)

# ---------------------------------------------------------
# Optional logging support (spdlog is fetched in root)
# ---------------------------------------------------------
if(ENABLE_LOGGING)
    target_link_libraries(connect4_client PRIVATE spdlog::spdlog)
    target_compile_definitions(connect4_client PRIVATE ENABLE_LOGGING)
endif()