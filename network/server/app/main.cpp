#include <iostream>
#include <csignal>
#include <cstdlib>
#include <string>

#include "protocol/protocol.hpp"
#include "server/game_server.hpp"

// Global server pointer for signal handler
GameServer *g_server = nullptr;

/**
 * Signal handler for graceful shutdown
 */
void signalHandler(int signal)
{
    std::cout << "\n\nReceived signal " << signal << " (";

    switch (signal)
    {
    case SIGINT:
        std::cout << "SIGINT";
        break;
    case SIGTERM:
        std::cout << "SIGTERM";
        break;
    default:
        std::cout << "UNKNOWN";
        break;
    }

    std::cout << ")" << std::endl;
    std::cout << "Shutting down gracefully..." << std::endl;

    if (g_server)
    {
        g_server->stop();
    }

    exit(0);
}

/**
 * Print usage information
 */
void printUsage(const char *program_name)
{
    std::cout << "Usage: " << program_name << " [PORT]" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  PORT    Port number to listen on (default: "
              << Protocol::DEFAULT_PORT << ")" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << "           # Use default port "
              << Protocol::DEFAULT_PORT << std::endl;
    std::cout << "  " << program_name << " 9000      # Use port 9000" << std::endl;
}

int main(int argc, char *argv[])
{
    // ========================================================================
    // Parse command line arguments
    // ========================================================================

    uint16_t port = Protocol::DEFAULT_PORT;

    if (argc > 1)
    {
        std::string arg = argv[1];

        // Check for help flag
        if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
            return 0;
        }

        // Parse port number
        try
        {
            int port_num = std::stoi(arg);

            if (port_num < 1 || port_num > 65535)
            {
                std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
                return 1;
            }

            port = static_cast<uint16_t>(port_num);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: Invalid port number '" << arg << "'" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // ========================================================================
    // Print banner
    // ========================================================================

    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "      Connect 4 Game Server" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Version:          0.1" << std::endl;
    std::cout << "Protocol Version: " << Protocol::VERSION << std::endl;
    std::cout << "Port:             " << port << std::endl;
    std::cout << "Max Message Size: " << Protocol::MAX_MESSAGE_SIZE << " bytes" << std::endl;
    std::cout << "Max Players:      " << (int)Protocol::MAX_PLAYERS << " per game" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // ========================================================================
    // Register signal handlers
    // ========================================================================

    std::signal(SIGINT, signalHandler);  // Ctrl+C
    std::signal(SIGTERM, signalHandler); // Termination request

#ifndef _WIN32
    // Ignore SIGPIPE on Unix systems (broken pipe when client disconnects)
    std::signal(SIGPIPE, SIG_IGN);
#endif

    // ========================================================================
    // Create and start server
    // ========================================================================

    try
    {
        GameServer server(port);
        g_server = &server;

        std::cout << "Press Ctrl+C to stop the server\n"
                  << std::endl;

        // Start server (blocking call)
        server.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nServer error: " << e.what() << std::endl;
        return 1;
    }

    // ========================================================================
    // Cleanup
    // ========================================================================

    std::cout << "\nServer terminated." << std::endl;
    return 0;
}