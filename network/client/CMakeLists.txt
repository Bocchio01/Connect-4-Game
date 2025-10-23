# ---------------------------------------------------------
# Client library
# ---------------------------------------------------------
add_library(connectx_client STATIC
    src/client.cpp
)

add_library(Connectx::client ALIAS connectx_client)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connectx_client
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------
# Link dependencies (sockpp and Threads are fetched/found in root)
# ---------------------------------------------------------
target_link_libraries(connectx_client
    PUBLIC
    Connectx::core
    Connectx::protocol
    sockpp
    Threads::Threads
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(connectx_client PUBLIC ws2_32)
endif()

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connectx_client PUBLIC cxx_std_17)
set_target_properties(connectx_client PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_EXTENSIONS OFF
)

# ---------------------------------------------------------
# Optional logging support (spdlog is fetched in root)
# ---------------------------------------------------------
if(ENABLE_LOGGING)
    target_link_libraries(connectx_client PRIVATE spdlog::spdlog)
    target_compile_definitions(connectx_client PRIVATE ENABLE_LOGGING)
endif()