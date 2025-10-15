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

CLIInterface::CLIInterface()
    : running_(false),
      waiting_for_input_(false),
      server_host_("localhost"),
      server_port_(8080),
      join_mode_(JoinMode::AUTO),
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

    // Print banner
    printBanner();

    // Setup callbacks
    setupCallbacks();

    // Connect to server
    if (!connectToServer())
    {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    // Get player name if not provided
    if (player_name_.empty())
    {
        std::cout << "\nEnter your name: ";
        std::getline(std::cin, player_name_);

        if (player_name_.empty())
        {
            player_name_ = "Player";
        }
    }

    // Start event loop in separate thread
    running_ = true;
    event_thread_ = std::thread(
        [this]()
        { client_.run(); });

    // Send connect request
    std::cout << "Connecting as '" + player_name_ + "'..." << std::endl;

    if (!client_.sendConnectRequest(player_name_))
    {
        std::cerr << "Failed to send connect request" << std::endl;
        return 1;
    }

    // Wait for connection response
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
              << "Thank you for playing Connect 4!" << std::endl;

    return 0;
}

void CLIInterface::handleJoinMode()
{
    switch (join_mode_)
    {
    case JoinMode::AUTO:
        autoJoinGame();
        break;

    case JoinMode::BY_ID:
        if (target_game_id_.has_value())
        {
            joinGameById(target_game_id_.value());
        }
        break;

    case JoinMode::BY_NAME:
        if (target_game_name_.has_value())
        {
            joinGameByName(target_game_name_.value());
        }
        break;

    case JoinMode::CREATE_CUSTOM:
        createCustomGame();
        break;

    case JoinMode::CREATE_AI:
        createAIGame();
        break;
    }
}

void CLIInterface::autoJoinGame()
{
    std::cout << "Looking for available games..." << std::endl;

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

    // Find first game waiting for players and not opened with a name (private)
    std::optional<uint32_t> target_game;
    for (const auto &game : available_games)
    {
        std::cout << "Checking game ID " << game.game_id << " (" << game.current_players << "/" << static_cast<int>(game.config.num_players) << " players)" << std::endl;
        std::cout << " Game name: '" << game.game_name << "'" << std::endl;
        if (game.current_players < game.config.num_players &&
            game.status == ProtocolGameStatus::NOT_STARTED &&
            game.game_name == "")
        {
            target_game = game.game_id;
            break;
        }
    }

    if (target_game.has_value())
    {
        std::cout << "Joining game " + std::to_string(target_game.value()) << std::endl;
        client_.sendJoinGame(target_game.value());
        return;
    }

    // No available games, create a new one
    GameConfig config(6, 7, 2, 4);
    std::cout << "No available games. Creating a new game..." << std::endl;
    client_.sendCreateGame(config);
}

void CLIInterface::joinGameById(uint32_t game_id)
{
    std::cout << "Joining game ID " + std::to_string(game_id) + "..." << std::endl;
    client_.sendJoinGame(game_id);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto info = client_.getGameInfo();
    if (!info.has_value() || info->game_id != game_id)
    {
        std::cerr << "Failed to join game ID " + std::to_string(game_id) << std::endl;
        return;
    }
}

void CLIInterface::joinGameByName(const std::string &name)
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
        return;
    }

    // Find game by name
    std::optional<uint32_t> target_game;
    for (const auto &game : available_games)
    {
        std::cout << "Checking game: " << game.game_name << std::endl;
        if (game.game_name == name &&
            game.current_players < game.config.num_players &&
            game.status == ProtocolGameStatus::NOT_STARTED)
        {
            target_game = game.game_id;
            break;
        }
    }

    if (target_game.has_value())
    {
        std::cout << "Found game! Joining..." << std::endl;
        client_.sendJoinGame(target_game.value());
    }
    else
    {
        std::cerr << "Game '" + name + "' not found or full" << std::endl;
    }
}

void CLIInterface::createCustomGame()
{
    if (!custom_game_spec_.has_value())
    {
        std::cerr << "No game specification provided" << std::endl;
        return;
    }

    const auto &spec = custom_game_spec_.value();

    std::cout << "Creating game '" + spec.name + "' (" + std::to_string(spec.rows) + "x" + std::to_string(spec.cols) + ", " + std::to_string(spec.num_players) + " players, " + std::to_string(spec.connect_length) + " to win)..." << std::endl;

    GameConfig config(spec.rows, spec.cols, spec.num_players, spec.connect_length);

    if (!client_.sendCreateGame(config, spec.name))
    {
        std::cerr << "Failed to create game" << std::endl;
        return;
    }

    // Wait a bit for response
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Game created! Game ID: " + std::to_string(client_.getGameId()) << std::endl;
    std::cout << "Waiting for " + std::to_string(spec.num_players - 1) + " more players..." << std::endl;
}

void CLIInterface::createAIGame()
{
    std::cout << "Creating game with AI opponent..." << std::endl;

    GameConfig config(6, 7, 2, 4); // Standard game

    if (!client_.sendCreateGame(config))
    {
        std::cerr << "Failed to create game" << std::endl;
        return;
    }

    // Wait for response
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Game created! Game ID: " + std::to_string(client_.getGameId()) << std::endl;
    std::cout << " AI opponent not yet implemented. Please launch another client manually." << std::endl;
    std::cout << " Run: ./connect4-cli -g " + std::to_string(client_.getGameId()) << std::endl;
}

void CLIInterface::setupCallbacks()
{

    // Connection established
    client_.onConnected(
        [this]()
        { std::cout << "Connected to server" << std::endl; });

    // Disconnected
    client_.onDisconnected(
        [this]()
        {
            std::cout << "\n"
                      << "Disconnected from server" << std::endl;
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
                std::cout << "\nGame is starting soon..." << std::endl;
                std::cout << "Enrolled players: " << std::to_string(state.players.size()) << "/" << std::to_string(client_.getGameInfo()->config.num_players) << std::endl;
                std::cout << "\nPlayers: " << std::endl;
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
                break;

            case ProtocolGameStatus::FINISHED_DRAW:
                system(CLEAR_COMMAND);
                printBoard(state);
                std::cout << "It's a DRAW!" << std::endl;
                return;
            }
        });

    // Move result
    client_.onMoveResult(
        [this](bool success, const std::string &message)
        {
            if (!success)
            {
                std::cout << "\nInvalid move: " + message << std::endl;
                std::cout << "Try again: ";
                std::cout.flush();
            }
        });

    // Game over
    client_.onGameOver(
        [this](const GameOverMessage &msg)
        {
            // Wait a bit then stop
            std::this_thread::sleep_for(std::chrono::seconds(2));
            running_ = false;
            client_.stop();
        });

    // Error
    client_.onError(
        [this](uint16_t code, const std::string &message)
        {
            std::cerr << "Server error " + std::to_string(code) + ": " + message << std::endl;
            if (client_.isMyTurn())
            {
                waiting_for_input_ = true;
            }
        });
}

bool CLIInterface::connectToServer()
{
    std::cout << "Connecting to " << server_host_ + ":" + std::to_string(server_port_) << "..." << std::endl;

    if (!client_.connect(server_host_, server_port_))
    {
        return false;
    }

    return true;
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

        // TODO CLean the input buffer before taking new input
        std::cin.clear();
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
