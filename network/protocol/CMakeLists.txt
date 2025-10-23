# ---------------------------------------------------------
# Library definition
# ---------------------------------------------------------
add_library(connectx_protocol STATIC
    src/messagetype.cpp
    src/serializer.cpp
)

add_library(Connectx::protocol ALIAS connectx_protocol)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connectx_protocol
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------
# Dependencies (nlohmann_json is fetched in root)
# ---------------------------------------------------------
target_link_libraries(connectx_protocol PUBLIC nlohmann_json::nlohmann_json)

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connectx_protocol PUBLIC cxx_std_17)
set_target_properties(connectx_protocol PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_EXTENSIONS OFF
)