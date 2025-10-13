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
# Dependencies
# ---------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)
target_link_libraries(connect4_protocol PUBLIC nlohmann_json::nlohmann_json)

# ---------------------------------------------------------
# Compiler setup
# ---------------------------------------------------------
# target_compile_features(connect4_protocol PUBLIC cxx_std_17)
set_target_properties(connect4_protocol PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(connect4_protocol PRIVATE /W4)
else()
    target_compile_options(connect4_protocol PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------
# Installation (optional, controlled by parent)
# ---------------------------------------------------------
if(INSTALL_PROTOCOL)
    include(GNUInstallDirs)

    install(TARGETS connect4_protocol
        EXPORT Connect4ProtocolTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT Connect4ProtocolTargets
        FILE Connect4ProtocolTargets.cmake
        NAMESPACE Connect4::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Connect4Protocol
    )
endif()

# ---------------------------------------------------------
# Optional example executables (optional, controlled by parent)
# ---------------------------------------------------------
if(BUILD_EXAMPLES)
    file(GLOB EXAMPLE_SOURCES examples/*.cpp)

    foreach(example_src ${EXAMPLE_SOURCES})
        get_filename_component(example_name ${example_src} NAME_WE)
        add_executable(${example_name} ${example_src})
        target_link_libraries(${example_name} PRIVATE connect4_protocol)
        target_include_directories(${example_name}
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
        )
        message(STATUS "Added example: ${example_name}")
    endforeach()
endif()