cmake_minimum_required(VERSION 3.16)
project(Connect4 VERSION 0.1 LANGUAGES CXX)

# =========================================================
# Build options
# =========================================================
option(BUILD_CORE "Build core library" ON)
option(BUILD_PROTOCOL "Build protocol library" ON)
option(BUILD_SERVER "Build server" ON)
option(BUILD_CLI "Build CLI client" OFF)
option(BUILD_AI "Build AI client" OFF)
option(BUILD_GUI "Build GUI client" OFF)

option(BUILD_TESTS "Build test executables" OFF)
option(BUILD_EXAMPLES "Build example executables" OFF)
option(ENABLE_LOGGING "Enable spdlog logging" OFF)

option(INSTALL_CORE "Install core library" OFF)
option(INSTALL_PROTOCOL "Install protocol library" OFF)
option(INSTALL_SERVER "Install server executable" OFF)

option(GENERATE_DOCS "Generate documentation with Doxygen" OFF)

# =========================================================
# Global settings
# =========================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =========================================================
# Prevent in-source builds
# =========================================================
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Please create a build directory.")
endif()

# =========================================================
# Output directories
# =========================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# =========================================================
# Optional: spdlog (if logging enabled)
# =========================================================
if(ENABLE_LOGGING)
    message(STATUS "Logging enabled - fetching spdlog...")
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
    )
    FetchContent_MakeAvailable(spdlog)
endif()

# =========================================================
# Add subdirectories
# =========================================================

# Core library (must be built first)
if(BUILD_CORE)
    message(STATUS "Adding core library...")
    add_subdirectory(core)
endif()

# Protocol library (depends on nothing, but used by server and clients)
if(BUILD_PROTOCOL)
    message(STATUS "Adding protocol library...")
    add_subdirectory(protocol)
endif()

# Server (depends on core and protocol)
if(BUILD_SERVER)
    if(NOT BUILD_CORE OR NOT BUILD_PROTOCOL)
        message(FATAL_ERROR "Server requires both core and protocol libraries")
    endif()

    message(STATUS "Adding server...")
    add_subdirectory(server)
endif()

# CLI client (depends on core and protocol)
if(BUILD_CLI)
    if(NOT BUILD_CORE OR NOT BUILD_PROTOCOL)
        message(FATAL_ERROR "CLI client requires both core and protocol libraries")
    endif()

    message(STATUS "Adding CLI client...")
    add_subdirectory(cli)
endif()

# AI client (depends on core and protocol)
if(BUILD_AI)
    if(NOT BUILD_CORE OR NOT BUILD_PROTOCOL)
        message(FATAL_ERROR "AI client requires both core and protocol libraries")
    endif()

    message(STATUS "Adding AI client...")
    add_subdirectory(ai)
endif()

# GUI client (depends on core and protocol)
if(BUILD_GUI)
    if(NOT BUILD_CORE OR NOT BUILD_PROTOCOL)
        message(FATAL_ERROR "GUI client requires both core and protocol libraries")
    endif()

    message(STATUS "Adding GUI client...")
    add_subdirectory(gui)
endif()

# =========================================================
# Testing setup
# =========================================================
if(BUILD_TESTS)
    enable_testing()
    message(STATUS "Testing enabled - use 'ctest' to run tests")
    add_subdirectory(tests)
endif()

# =========================================================
# Examples setup
# =========================================================
if(BUILD_EXAMPLES)
    message(STATUS "Adding examples...")
    add_subdirectory(examples)
endif()

# =========================================================
# Documentation setup
# =========================================================
find_package(Doxygen QUIET)

if(DOXYGEN_FOUND AND GENERATE_DOCS)
    set(DOXYGEN_INPUT ${CMAKE_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUTPUT ${CMAKE_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_INPUT} ${DOXYGEN_OUTPUT} @ONLY)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUTPUT}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    message(STATUS "Doxygen documentation target available: `make docs` or `cmake --build . --target docs`")
elseif(GENERATE_DOCS AND NOT DOXYGEN_FOUND)
    message(WARNING "Doxygen not found. Install it to enable documentation generation.")
endif()

# =========================================================
# Print configuration summary
# =========================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  Connect 4 Configuration")
message(STATUS "========================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Components:")
message(STATUS "  Core library:      ${BUILD_CORE}")
message(STATUS "  Protocol library:  ${BUILD_PROTOCOL}")
message(STATUS "  Server:            ${BUILD_SERVER}")
message(STATUS "  CLI client:        ${BUILD_CLI}")
message(STATUS "  AI client:         ${BUILD_AI}")
message(STATUS "  GUI client:        ${BUILD_GUI}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  Build tests:       ${BUILD_TESTS}")
message(STATUS "  Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "  Enable logging:    ${ENABLE_LOGGING}")
message(STATUS "")
message(STATUS "Output:")
message(STATUS "  Executables:       ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Libraries:         ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "========================================")
message(STATUS "")