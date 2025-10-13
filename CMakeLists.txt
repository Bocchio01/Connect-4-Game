cmake_minimum_required(VERSION 3.16)
project(connect4 VERSION 0.1 LANGUAGES CXX)

# =========================
# Global build settings
# =========================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_CORE "Build the core game engine" ON)
option(BUILD_SERVER "Build the server" OFF)
option(BUILD_CLIENT "Build the client" OFF)
option(BUILD_AI "Build the AI module" OFF)
option(BUILD_UI "Build the user interfaces" OFF)
option(BUILD_TESTS "Build unit tests" ON)

# Optional stricter defaults
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Please use a separate build directory.")
endif()

# =========================
# Subdirectories
# =========================
if(BUILD_CORE)
    add_subdirectory(core)
endif()

if(BUILD_SERVER OR BUILD_CLIENT)
    add_subdirectory(protocol)
endif()

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BUILD_AI)
    add_subdirectory(ai)
endif()

if(BUILD_UI)
    add_subdirectory(ui)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

find_package(Doxygen QUIET)
option(GENERATE_DOCS "Generate documentation with Doxygen" OFF)

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
