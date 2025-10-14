add_subdirectory(protocol)
add_subdirectory(server)
add_subdirectory(client)

# =========================================================
# Combined Network Library
# =========================================================
add_library(connect4_network INTERFACE)
add_library(Connect4::network ALIAS connect4_network)

target_link_libraries(connect4_network
    INTERFACE
    Connect4::protocol
    Connect4::server_lib
    Connect4::client
)