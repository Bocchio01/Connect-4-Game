#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <regex>
#include <limits>
#include <map>

#include "cli/interface.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef max
#undef min
#define CLEAR_COMMAND "cls"
#else
#include <unistd.h>
#define CLEAR_COMMAND "clear"
#endif

// #ifdef _WIN32
// #include <conio.h>
// #else
// #include <sys/ioctl.h>
// #include <unistd.h>
// #include <termios.h>
// #endif

// namespace
// {
//     bool isInputAvailable()
//     {
// #ifdef _WIN32
//         return _kbhit();
// #else
//         int bytesWaiting;
//         ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
//         return bytesWaiting > 0;
// #endif
//     }
// }

CLIInterface::CLIInterface()
    : running_(false),
      waiting_for_input_(false),
      server_host_("localhost"),
      server_port_(8080),
      join_mode_(JoinMode::AUTO),
      request_game_list_(false),
      with_ai_(false)
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

    // Setup callbacks
    setupCallbacks();

    // Connect to server
    if (!client_.connect(server_host_, server_port_))
    {
        std::cerr << "Failed to connect to server at "
                  << server_host_ + ":" + std::to_string(server_port_) << std::endl;
        return EXIT_FAILURE;
    }

    // Start event loop in separate thread
    running_ = true;
    event_thread_ = std::thread([this]()
                                { client_.run(); });

    if (request_game_list_)
    {
        // Request and print game list, then exit
        std::atomic<bool> list_received = false;

        client_.onGameList(
            [&](const std::vector<GameInfo> &games)
            {
                printGameList(games);
                list_received = true;
                client_.stop();
            });

        client_.sendListGames();

        // Wait for list
        auto start = std::chrono::steady_clock::now();
        while (!list_received &&
               std::chrono::steady_clock::now() - start < std::chrono::seconds(3))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!list_received)
        {
            std::cerr << "Failed to get game list" << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    // Get player name if not provided
    if (player_name_.empty())
    {
        std::cout << "\nEnter your name (default: Player): ";
        std::getline(std::cin, player_name_);

        if (player_name_.empty())
            player_name_ = "Player";
    }

    // Send connect request
    if (!client_.sendConnectRequest(player_name_))
    {
        std::cerr << "Failed to send connect request" << std::endl;
        return EXIT_FAILURE;
    }

    // Wait for connection response (session token set in callback)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Handle join mode
    handleJoinMode();

    // Handle user input in main thread
    handleUserInput();

    // Wait for event thread to finish
    if (event_thread_.joinable())
    {
        event_thread_.join();
    }

    std::cout << "\n"
              << "Thank you for playing Connect X!" << std::endl;

    return 0;
}

void CLIInterface::handleJoinMode()
{
    switch (join_mode_)
    {
    case JoinMode::AUTO:
        joinGame();
        break;

    case JoinMode::BY_ID:
        if (target_game_id_.has_value())
        {
            joinGame(target_game_id_.value());
        }
        break;

    case JoinMode::BY_NAME:
        if (target_game_name_.has_value())
        {
            joinGame(target_game_name_.value());
        }
        break;

    case JoinMode::CREATE_CUSTOM:
        createCustomGame();
        joinGame(client_.getGameId());
        break;
    }
}

void CLIInterface::joinGame()
{
    std::cout << "Auto-joining first available game..." << std::endl;
    client_.sendJoinGame(0); // 0 indicates auto-join

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto info = client_.getGameInfo();
    if (!info.has_value())
    {
        std::cerr << "Failed to join any game" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void CLIInterface::joinGame(uint32_t game_id)
{
    std::cout << "Joining game ID(" + std::to_string(game_id) + ") ..." << std::endl;
    client_.sendJoinGame(game_id);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto info = client_.getGameInfo();
    if (!info.has_value() || info->game_id != game_id)
    {
        std::cerr << "Failed to join game ID " + std::to_string(game_id) << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void CLIInterface::joinGame(const std::string &name)
{
    std::cout << "Looking for game '" + name + "'..." << std::endl;

    // Request game list
    std::atomic<bool> list_received = false;
    std::vector<GameInfo> available_games;

    client_.onGameList(
        [&](const std::vector<GameInfo> &games)
        {
            available_games = games;
            list_received = true;
        });

    client_.sendListGames();

    // Wait for list
    auto start = std::chrono::steady_clock::now();
    while (!list_received &&
           std::chrono::steady_clock::now() - start < std::chrono::seconds(3))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!list_received)
    {
        std::cerr << "Failed to get game list" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Find game by name
    std::optional<uint32_t> target_game;
    for (const auto &game : available_games)
    {
        if (game.game_name == name &&
            game.current_players < game.config.num_players &&
            game.status == ProtocolGameStatus::NOT_STARTED)
        {
            target_game = game.game_id;
            break;
        }
    }

    if (!target_game.has_value())
    {
        std::cerr << "Game '" + name + "' not found or full" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Found game! Joining..." << std::endl;
    client_.sendJoinGame(target_game.value());
}

void CLIInterface::createCustomGame()
{
    if (!custom_game_spec_.has_value())
    {
        std::cerr << "No game specification provided" << std::endl;
        return;
    }

    const auto &spec = custom_game_spec_.value();

    std::cout << "Creating game: " << spec.name << "<"
              << (int)spec.rows << ", "
              << (int)spec.cols << ", "
              << (int)spec.num_players << ", "
              << (int)spec.connect_length << "> ..." << std::endl;

    GameConfig config(spec.rows, spec.cols, spec.num_players, spec.connect_length);

    if (!client_.sendCreateGame(config, spec.name))
    {
        std::cerr << "Failed to create game" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Wait a bit for response
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (client_.getGameInfo().has_value() == false)
    {
        std::cerr << "Failed to create game" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Game created successfully!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void CLIInterface::setupCallbacks()
{
    // Disconnected
    client_.onDisconnected(
        [this]()
        {
            waiting_for_input_ = false;
            // std::cout << "\n"
            //           << "Disconnected from server" << std::endl;

            client_.stop();
        });

    // Game state update
    client_.onGameStateUpdate(
        [this](const GameStateUpdate &state)
        {
            switch (state.status)
            {

            case ProtocolGameStatus::NOT_STARTED:
                system(CLEAR_COMMAND);
                std::cout << "Game is starting soon..." << std::endl;
                std::cout << "Players (" << std::to_string(state.players.size()) << "/" << std::to_string(client_.getGameInfo()->config.num_players) << "):" << std::endl;
                for (size_t i = 0; i < state.players.size(); ++i)
                {
                    std::cout << "- " + state.player_names[i] << std::endl;
                }
                std::cout << std::endl;
                break;

            case ProtocolGameStatus::IN_PROGRESS:
                system(CLEAR_COMMAND);
                printBoard(state);

                if (client_.isMyTurn())
                {
                    waiting_for_input_ = true;
                }
                else
                {
                    std::cout << "Waiting for "
                              << state.player_names[state.current_player - 1]
                              << "'s move..." << std::endl;
                    waiting_for_input_ = false;
                }
                break;

            case ProtocolGameStatus::FINISHED_WIN:
                system(CLEAR_COMMAND);
                printBoard(state);
                if (state.winner.value() == client_.getPlayerId())
                {
                    std::cout << "YOU WIN!" << std::endl;
                }
                else
                {
                    std::cout << "Player " + state.player_names[state.winner.value() - 1] + " wins!" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::seconds(2));
                running_ = false;
                waiting_for_input_ = false;
                client_.stop();
                break;

            case ProtocolGameStatus::FINISHED_DRAW:
                system(CLEAR_COMMAND);
                printBoard(state);
                std::cout << "It's a DRAW!" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                running_ = false;
                waiting_for_input_ = false;
                client_.stop();
                break;
            }
        });

    // Move result
    client_.onMoveResult(
        [this](bool success, const std::string &message)
        {
            if (!success)
            {
                std::cout << "\nTry again: " + message << std::endl;
                std::cout.flush();
            }
            if (client_.isMyTurn())
            {
                waiting_for_input_ = true;
            }
        });

    // Error
    client_.onError(
        [this](uint16_t code, const std::string &message)
        {
            std::cerr << "Error ID(" + std::to_string(code) + "): " + message << std::endl;
        });
}

void CLIInterface::handleUserInput()
{
    while (running_ && client_.isConnected())
    {
        if (!waiting_for_input_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // TODO: Clear any leftover input
        // std::cin.clear();
        // if (isInputAvailable())
        // {
        //     std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        // }
        std::cout << "Enter column (0-" << (int)(client_.getGameInfo()->config.cols - 1) << "): ";

        int column;
        if (!(std::cin >> column))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Try again.\n";
            continue;
        }

        if (!client_.sendMove(static_cast<uint8_t>(column)))
        {
            std::cerr << "Failed to send move\n";
        }

        waiting_for_input_ = false;
    }
}
