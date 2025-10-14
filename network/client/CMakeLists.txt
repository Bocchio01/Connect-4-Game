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
# Link dependencies
# ---------------------------------------------------------
target_link_libraries(connect4_client
    PUBLIC
    Connect4::core
    Connect4::protocol
    sockpp
)

# Add sockpp include directory manually
target_include_directories(connect4_client PUBLIC
    ${sockpp_SOURCE_DIR}/include
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

if(MSVC)
    target_compile_options(connect4_client PRIVATE /W4)
else()
    target_compile_options(connect4_client PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------
# Optional logging support (inherits option from root)
# ---------------------------------------------------------
if(ENABLE_LOGGING)
    target_link_libraries(connect4_client PRIVATE spdlog::spdlog)
    target_compile_definitions(connect4_client PRIVATE ENABLE_LOGGING)
endif()

# ---------------------------------------------------------
# Installation (optional, controlled by parent)
# ---------------------------------------------------------
if(INSTALL_CLIENT)
    include(GNUInstallDirs)

    install(TARGETS connect4_client
        EXPORT Connect4ClientTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT Connect4ClientTargets
        FILE Connect4ClientTargets.cmake
        NAMESPACE Connect4::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Connect4Client
    )
endif()
