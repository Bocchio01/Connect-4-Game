# ---------------------------------------------------------
# Client executable
# ---------------------------------------------------------
add_executable(connect4_cli
    app/main.cpp
)

target_link_libraries(connect4_cli
    PRIVATE
    Connect4::core
    Connect4::client
)

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
