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

# ---------------------------------------------------------
# Client executable
# ---------------------------------------------------------
add_executable(connect4_cli
    app/main.cpp
)

target_link_libraries(connect4_cli
    PRIVATE
    Connect4::protocol
    sockpp
)

if(WIN32)
    target_link_libraries(connect4_cli PRIVATE ws2_32)
endif()

# ---------------------------------------------------------
# Warnings and options
# ---------------------------------------------------------
if(MSVC)
    target_compile_options(connect4_cli PRIVATE /W4)
else()
    target_compile_options(connect4_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------
# Output name
# ---------------------------------------------------------
set_target_properties(connect4_cli PROPERTIES
    OUTPUT_NAME "connect4-cli"
)
