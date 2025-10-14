#include <iostream>
#include <thread>
#include <chrono>

#include "client/client.hpp"

void printBoard(const std::vector<std::vector<uint8_t>> &board)
{
    std::cout << "\n  ";
    for (size_t c = 0; c < board[0].size(); ++c)
    {
        std::cout << c << " ";
    }
    std::cout << "\n ╔";
    for (size_t c = 0; c < board[0].size(); ++c)
    {
        std::cout << "═";
        if (c < board[0].size() - 1)
            std::cout << "╤";
    }
    std::cout << "╗\n";

    for (size_t r = 0; r < board.size(); ++r)
    {
        std::cout << " ║";
        for (size_t c = 0; c < board[r].size(); ++c)
        {
            if (board[r][c] == 0)
            {
                std::cout << "·";
            }
            else
            {
                std::cout << (char)('0' + board[r][c]);
            }
            if (c < board[r].size() - 1)
                std::cout << "│";
        }
        std::cout << "║\n";

        if (r < board.size() - 1)
        {
            std::cout << " ╟";
            for (size_t c = 0; c < board[r].size(); ++c)
            {
                std::cout << "─";
                if (c < board[r].size() - 1)
                    std::cout << "┼";
            }
            std::cout << "╢\n";
        }
    }

    std::cout << " ╚";
    for (size_t c = 0; c < board[0].size(); ++c)
    {
        std::cout << "═";
        if (c < board[0].size() - 1)
            std::cout << "╧";
    }
    std::cout << "╝\n"
              << std::endl;
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  Connect 4 Client Example" << std::endl;
    std::cout << "========================================\n"
              << std::endl;

    // Create client
    Client client;

    // Setup callbacks
    client.onConnected([&](uint8_t player_id, uint32_t game_id)
                       {
        std::cout << "✓ Connected!" << std::endl;
        std::cout << "  You are Player " << (int)player_id << std::endl;
        std::cout << "  Game ID: " << game_id << std::endl;
        std::cout << "  Waiting for other players...\n" << std::endl; });

    client.onDisconnected([]()
                          { std::cout << "\n✗ Disconnected from server" << std::endl; });

    client.onGameStateUpdate([&](const GameStateUpdate &state)
                             {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "Game State Update" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;

        printBoard(state.board);

        std::cout << "Players in game: ";
        for (uint8_t p : state.players) {
            std::cout << (int)p << " ";
        }
        std::cout << std::endl;

        if (state.current_player == client.getPlayerId()) {
            std::cout << "\n>>> YOUR TURN! <<<" << std::endl;
            std::cout << "Enter column (0-" << (int)(state.cols - 1) << "): ";
            std::cout.flush();
        } else {
            std::cout << "\nWaiting for Player " << (int)state.current_player
                      << "'s move..." << std::endl;
        } });

    client.onMoveResult([](bool success, const std::string &message)
                        {
        if (success) {
            std::cout << "✓ Move accepted!" << std::endl;
        } else {
            std::cout << "✗ Move rejected: " << message << std::endl;
        } });

    client.onGameOver([&](const GameOverMessage &msg)
                      {
        std::cout << "\n" << std::string(40, '=') << std::endl;
        std::cout << "        GAME OVER" << std::endl;
        std::cout << std::string(40, '=') << std::endl;

        if (msg.winner.has_value()) {
            if (msg.winner.value() == client.getPlayerId()) {
                std::cout << "\n🎉 YOU WIN! 🎉\n" << std::endl;
            } else {
                std::cout << "\nPlayer " << (int)msg.winner.value()
                          << " wins!" << std::endl;
            }
        } else {
            std::cout << "\nIt's a draw!" << std::endl;
        }

        std::cout << msg.message << std::endl;
        std::cout << std::string(40, '=') << "\n" << std::endl;

        // Disconnect after game over
        std::this_thread::sleep_for(std::chrono::seconds(2));
        client.stop(); });

    client.onError([](uint16_t code, const std::string &message)
                   { std::cerr << "✗ Error " << code << ": " << message << std::endl; });

    // Connect to server
    std::string host = "localhost";
    uint16_t port = 8080;

    std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;

    if (!client.connect(host, port))
    {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    // Get player name
    std::string name;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);

    if (name.empty())
    {
        name = "Player";
    }

    // Send connect request
    if (!client.sendConnectRequest(name))
    {
        std::cerr << "Failed to send connect request" << std::endl;
        return 1;
    }

    // Start event loop in separate thread
    std::thread event_thread([&client]()
                             { client.run(); });

    // Main input loop
    while (client.isConnected())
    {
        // Wait for it to be our turn
        while (client.isConnected() && !client.isMyTurn())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!client.isConnected())
        {
            break;
        }

        // Get move from user
        int column;
        if (!(std::cin >> column))
        {
            // Input error or EOF
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }
        std::cin.ignore(10000, '\n'); // Clear rest of line

        // Send move
        if (!client.sendMove(static_cast<uint8_t>(column)))
        {
            std::cerr << "Failed to send move" << std::endl;
        }
    }

    // Wait for event thread to finish
    if (event_thread.joinable())
    {
        event_thread.join();
    }

    std::cout << "\nThank you for playing!" << std::endl;

    return 0;
}