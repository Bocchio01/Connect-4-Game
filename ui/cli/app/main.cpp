#include <iostream>
#include <csignal>
#include <thread>
#include <string>

#include <sockpp/tcp_connector.h>
#include "protocol/protocol.hpp"
#include "protocol/messages.hpp"
#include "server/connection.hpp"

static bool running = true;

void signalHandler(int signal)
{
    std::cout << "\nReceived signal " << signal << " — shutting down client..." << std::endl;
    running = false;
}

int main(int argc, char *argv[])
{
    std::string host = "127.0.0.1";
    uint16_t port = Protocol::DEFAULT_PORT;
    std::string playerName = "Player1";

    if (argc > 1)
        host = argv[1];
    if (argc > 2)
        port = static_cast<uint16_t>(std::stoi(argv[2]));
    if (argc > 3)
        playerName = argv[3];

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "Connecting to server at " << host << ":" << port << "..." << std::endl;

    // Initialize sockpp
    sockpp::socket_initializer::initialize();

    // Connect to server
    sockpp::tcp_connector connector({host, port});
    if (!connector)
    {
        std::cerr << "Error connecting to server: " << connector.last_error_str() << std::endl;
        return 1;
    }

    std::cout << "Connected successfully!" << std::endl;

    // Wrap in Connection object
    Connection conn(std::move(connector), 1);

    // Prepare CONNECT_REQUEST
    ConnectRequest req;
    req.player_name = playerName;

    // Serialize and wrap it
    std::string payload = MessageSerializer::serialize(req);
    std::string message = MessageSerializer::wrapMessage(MessageType::CONNECT_REQUEST, payload);

    // Send it
    if (!conn.send(message))
    {
        std::cerr << "Failed to send CONNECT_REQUEST" << std::endl;
        conn.close();
        return 1;
    }

    std::cout << "CONNECT_REQUEST sent as '" << playerName << "'" << std::endl;

    // Wait for server responses (optional print)
    conn.setMessageCallback([](const std::string &msg)
                            {
        try {
            auto [type, payload] = MessageSerializer::unwrapMessage(msg);
            std::cout << "[Server -> Client] Received: " << messageTypeToString(type) << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
        } });

    conn.setDisconnectCallback([]()
                               {
        std::cout << "Disconnected from server." << std::endl;
        running = false; });

    // Run message loop
    std::thread reader([&conn]()
                       { conn.start(); });

    while (running && conn.isOpen())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    conn.close();
    if (reader.joinable())
        reader.join();

    std::cout << "Client terminated." << std::endl;
    return 0;
}
