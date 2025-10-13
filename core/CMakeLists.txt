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

if(MSVC)
    target_compile_options(connect4_core PRIVATE /W4)
else()
    target_compile_options(connect4_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------
# Optional logging support (inherits option from root)
# ---------------------------------------------------------
if(ENABLE_LOGGING)
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
    )
    FetchContent_MakeAvailable(spdlog)
    target_link_libraries(connect4_core PRIVATE spdlog::spdlog)
    target_compile_definitions(connect4_core PRIVATE ENABLE_LOGGING)
endif()

# ---------------------------------------------------------
# Installation (optional, controlled by parent)
# ---------------------------------------------------------
if(INSTALL_CORE)
    include(GNUInstallDirs)

    install(TARGETS connect4_core
        EXPORT Connect4CoreTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT Connect4CoreTargets
        FILE Connect4CoreTargets.cmake
        NAMESPACE Connect4::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Connect4Core
    )
endif()
