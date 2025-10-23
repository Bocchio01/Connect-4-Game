add_subdirectory(protocol)
add_subdirectory(server)
add_subdirectory(client)

# =========================================================
# Combined Network Library
# =========================================================
add_library(connectx_network INTERFACE)
add_library(Connectx::network ALIAS connectx_network)

target_link_libraries(connectx_network
    INTERFACE
    Connectx::protocol
    Connectx::server_lib
    Connectx::client
)