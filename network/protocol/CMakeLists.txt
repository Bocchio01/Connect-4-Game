# ---------------------------------------------------------
# Library definition
# ---------------------------------------------------------
add_library(connect4_protocol STATIC
    src/messagetype.cpp
    src/serializer.cpp
)

add_library(Connect4::protocol ALIAS connect4_protocol)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(connect4_protocol
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------
# Dependencies (nlohmann_json is fetched in root)
# ---------------------------------------------------------
target_link_libraries(connect4_protocol PUBLIC nlohmann_json::nlohmann_json)

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
target_compile_features(connect4_protocol PUBLIC cxx_std_17)
set_target_properties(connect4_protocol PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_EXTENSIONS OFF
)