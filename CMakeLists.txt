cmake_minimum_required(VERSION 3.16)
project(Connect4 VERSION 0.1 LANGUAGES CXX)

# =========================================================
# Build options
# =========================================================
option(BUILD_CORE "Build core library" ON)
option(BUILD_NETWORK "Build network components" ON)
option(BUILD_AI "Build AI client" OFF)
option(BUILD_CLI "Build CLI client" OFF)
option(BUILD_GUI "Build GUI client" OFF)

option(BUILD_TESTS "Build test executables" ON)
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

# Network components (depends on core)
if(BUILD_NETWORK)
    message(STATUS "Adding network components...")
    add_subdirectory(network)
endif()

# AI client (depends on core and network)
if(BUILD_AI)
    message(STATUS "Adding AI client...")
    add_subdirectory(ai)
endif()

# CLI client (depends on core and network)
if(BUILD_CLI)
    message(STATUS "Adding CLI client...")
    add_subdirectory(cli)
endif()

# GUI client (depends on core and network)
if(BUILD_GUI)
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
    include(FetchContent)
    FetchContent_Declare(
        doxygen-awesome-css
        URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/heads/main.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(doxygen-awesome-css)
    FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

    # Configure Doxygen
    set(DOXYGEN_INPUT ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUTPUT ${CMAKE_BINARY_DIR}/docs/Doxyfile)
    configure_file(${DOXYGEN_INPUT} ${DOXYGEN_OUTPUT} @ONLY)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUTPUT}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

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
message(STATUS "  Network:           ${BUILD_NETWORK}")
message(STATUS "  AI client:         ${BUILD_AI}")
message(STATUS "  CLI client:        ${BUILD_CLI}")
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