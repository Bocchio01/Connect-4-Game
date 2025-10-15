#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>

#include "cli/interface.hpp"

#ifdef _WIN32
#include <windows.h>
#define CLEAR_COMMAND "cls"
#else
#include <unistd.h>
#define CLEAR_COMMAND "clear"
#endif

// ANSI color codes
namespace Color
{
    const int RESET = 0;
    const int RED = 31;
    const int GREEN = 32;
    const int YELLOW = 33;
    const int BLUE = 34;
    const int MAGENTA = 35;
    const int CYAN = 36;
    const int WHITE = 37;
    const int BRIGHT_RED = 91;
    const int BRIGHT_GREEN = 92;
    const int BRIGHT_YELLOW = 93;
    const int BRIGHT_BLUE = 94;
}

CLIInterface::CLIInterface()
    : running_(false),
      waiting_for_input_(false),
      server_host_("localhost"),
      server_port_(8080)
{
}

CLIInterface::~CLIInterface()
{
    running_ = false;
    if (event_thread_.joinable())
    {
        event_thread_.join();
    }
}

int CLIInterface::run(int argc, char *argv[])
{
    // Parse command line arguments
    parseArguments(argc, argv);

    // Print banner
    printBanner();

    // Setup callbacks
    setupCallbacks();

    // Connect to server
    if (!connectToServer())
    {
        std::cerr << colorize("Failed to connect to server", Color::RED) << std::endl;
        return 1;
    }

    // Get player name
    if (player_name_.empty())
    {
        std::cout << "\nEnter your name: ";
        std::getline(std::cin, player_name_);

        if (player_name_.empty())
        {
            player_name_ = "Player";
        }
    }

    // Send connect request
    std::cout << colorize("Joining game as '" + player_name_ + "'...", Color::WHITE) << std::endl;

    if (!client_.sendConnectRequest(player_name_))
    {
        std::cerr << colorize("Failed to send connect request", Color::RED) << std::endl;
        return 1;
    }

    // Start event loop in separate thread
    running_ = true;
    event_thread_ = std::thread([this]()
                                { client_.run(); });

    // Handle user input in main thread
    handleUserInput();

    // Wait for event thread to finish
    if (event_thread_.joinable())
    {
        event_thread_.join();
    }

    std::cout << "\n"
              << colorize("Thank you for playing Connect 4!", Color::WHITE) << std::endl;

    return 0;
}

void CLIInterface::parseArguments(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            printHelp();
            exit(0);
        }
        else if (arg == "--host" && i + 1 < argc)
        {
            server_host_ = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc)
        {
            server_port_ = static_cast<uint16_t>(std::atoi(argv[++i]));
        }
        else if (arg == "--name" && i + 1 < argc)
        {
            player_name_ = argv[++i];
        }
    }
}

void CLIInterface::printBanner()
{
    clearScreen();
    std::cout << colorize("\n+========================================+", Color::WHITE) << std::endl;
    std::cout << colorize("|                                        |", Color::WHITE) << std::endl;
    std::cout << colorize("|              ", Color::WHITE)
              << colorize("CONNECT 4", Color::WHITE)
              << colorize("                 |", Color::WHITE) << std::endl;
    std::cout << colorize("|                                        |", Color::WHITE) << std::endl;
    std::cout << colorize("+========================================+", Color::WHITE) << std::endl;
    std::cout << std::endl;
}

void CLIInterface::printHelp()
{
    std::cout << "Connect 4 CLI Client\n"
              << std::endl;
    std::cout << "Usage: connect4-cli [OPTIONS]\n"
              << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --host <hostname>   Server hostname (default: localhost)" << std::endl;
    std::cout << "  --port <port>       Server port (default: 8080)" << std::endl;
    std::cout << "  --name <name>       Your player name" << std::endl;
    std::cout << "  -h, --help          Show this help message" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  connect4-cli" << std::endl;
    std::cout << "  connect4-cli --host game.example.com --port 9000" << std::endl;
    std::cout << "  connect4-cli --name Alice" << std::endl;
}

void CLIInterface::setupCallbacks()
{
    // Connection established
    client_.onConnected([this](uint8_t player_id, uint32_t game_id)
                        {
        std::cout << colorize("✓ Connected to game!", Color::GREEN) << std::endl;
        std::cout << "  You are Player " << colorize(std::to_string(player_id), Color::BRIGHT_YELLOW) << std::endl;
        std::cout << "  Game ID: " << game_id << std::endl;
        std::cout << colorize("\nWaiting for other players to join...", Color::YELLOW) << std::endl; });

    // Disconnected
    client_.onDisconnected([this]()
                           {
        std::cout << "\n" << colorize("Disconnected from server", Color::RED) << std::endl;
        running_ = false; });

    // Game state update
    client_.onGameStateUpdate([this](const GameStateUpdate &state)
                              {
        clearScreen();
        printBanner();

        std::cout << colorize("==========================================", Color::WHITE) << std::endl;
        std::cout << colorize("=          GAME IN PROGRESS              =", Color::WHITE) << std::endl;
        std::cout << colorize("==========================================", Color::WHITE) << std::endl;

        printBoard(state);

        // std::cout << "\nPlayers: ";
        // for (size_t i = 0; i < state.players.size(); ++i) {
        //     std::cout << colorize("Player " + std::to_string(state.players[i]), Color::BRIGHT_YELLOW);
        //     if (i < state.players.size() - 1) {
        //         std::cout << " vs ";
        //     }
        // }
        // std::cout << std::endl;

        if (client_.isMyTurn()) {
            // std::cout << "\n" << colorize(">>> YOUR TURN! <<<", Color::BRIGHT_GREEN) << std::endl;
            std::cout << "Enter column (0-" << (int)(state.cols - 1) << "): ";
            std::cout.flush();
            waiting_for_input_ = true;
        } else {
            std::cout << "\nWaiting for "
                      << colorize("Player " + std::to_string(state.current_player), Color::WHITE)
                      << "'s move..." << std::endl;
            waiting_for_input_ = false;
        } });

    // Move result
    client_.onMoveResult([this](bool success, const std::string &message)
                         {
        if (!success) {
            std::cout << colorize("\nInvalid move: " + message, Color::RED) << std::endl;
            std::cout << "Try again: ";
            std::cout.flush();
        } });

    // Game over
    client_.onGameOver([this](const GameOverMessage &msg)
                       {
        clearScreen();
        printBanner();

        printSeparator('=', 50);

        if (msg.winner.has_value()) {
            if (msg.winner.value() == client_.getPlayerId()) {
                std::cout << "\n" << colorize("YOU WIN!", Color::BRIGHT_GREEN) << std::endl;
            } else {
                std::cout << "\n" << colorize("Player " + std::to_string(msg.winner.value()) + " wins!", Color::YELLOW) << std::endl;
                std::cout << colorize("Better luck next time!", Color::WHITE) << std::endl;
            }
        } else {
            std::cout << "\n" << colorize("It's a DRAW!", Color::YELLOW) << std::endl;
        }

        std::cout << "\n" << msg.message << std::endl;
        printSeparator('=', 50);

        std::cout << "\nPress Enter to exit...";
        std::cout.flush();

        // Wait a bit then stop
        std::this_thread::sleep_for(std::chrono::seconds(2));
        running_ = false;
        client_.stop(); });

    // Error
    client_.onError([this](uint16_t code, const std::string &message)
                    { std::cerr << colorize("Server error " + std::to_string(code) + ": " + message, Color::RED) << std::endl; });
}

bool CLIInterface::connectToServer()
{
    std::cout << "Connecting to " << colorize(server_host_ + ":" + std::to_string(server_port_), Color::WHITE) << "..." << std::endl;

    if (!client_.connect(server_host_, server_port_))
    {
        return false;
    }

    return true;
}

void CLIInterface::printBoard(const GameStateUpdate &state)
{
    std::cout << std::endl;

    // Rows
    for (uint8_t r = 0; r < state.rows; ++r)
    {
        for (uint8_t c = 0; c < state.cols; ++c)
        {
            uint8_t cell = state.board[r][c];
            if (cell == 0)
            {
                std::cout << " . ";
            }
            else
            {
                int color = (cell == 1) ? Color::BRIGHT_RED : (cell == 2) ? Color::BRIGHT_BLUE
                                                          : (cell == 3)   ? Color::BRIGHT_GREEN
                                                                          : Color::WHITE;
                std::cout << " " << colorize(std::to_string(cell), color) << " ";
            }
        }
        std::cout << std::endl;
    }

    // Bottom border
    for (uint8_t c = 0; c < state.cols; ++c)
    {
        std::cout << "---";
    }

    // Column numbers
    std::cout << std::endl;
    for (uint8_t c = 0; c < state.cols; ++c)
    {
        std::cout << " " << colorize(std::to_string(c), Color::BRIGHT_YELLOW) << " ";
    }
    std::cout << std::endl;
}

void CLIInterface::handleUserInput()
{
    while (running_ && client_.isConnected())
    {
        // Only read input when it's our turn
        if (!waiting_for_input_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        int column = getColumnInput();

        if (column < 0)
        {
            // Invalid input or EOF
            continue;
        }

        // Send move
        if (!client_.sendMove(static_cast<uint8_t>(column)))
        {
            std::cerr << colorize("Failed to send move", Color::RED) << std::endl;
        }

        waiting_for_input_ = false;
    }
}

int CLIInterface::getColumnInput()
{
    std::string line;
    if (!std::getline(std::cin, line))
    {
        // EOF or error
        return -1;
    }

    // Try to parse as integer
    std::istringstream iss(line);
    int column;

    if (!(iss >> column))
    {
        std::cout << colorize("Invalid input. Please enter a number: ", Color::RED);
        std::cout.flush();
        return -1;
    }

    return column;
}

void CLIInterface::clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void CLIInterface::printSeparator(char ch, int length)
{
    std::cout << colorize(std::string(length, ch), Color::WHITE) << std::endl;
}

std::string CLIInterface::getPlayerSymbol(uint8_t player_id)
{
    switch (player_id)
    {
    case 1:
        return "●"; // Filled circle
    case 2:
        return "○"; // Empty circle
    case 3:
        return "◆"; // Diamond
    case 4:
        return "▲"; // Triangle
    default:
        return std::to_string(player_id);
    }
}

std::string CLIInterface::colorize(const std::string &text, int color_code)
{
    std::ostringstream oss;
    oss << "\033[" << color_code << "m" << text << "\033[0m";
    return oss.str();
}