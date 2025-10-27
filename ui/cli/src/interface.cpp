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
    : waiting_for_input_(false),
      server_host_("localhost"),
      server_port_(8080),
      join_mode_(JoinMode::AUTO),
      request_game_list_(false),
      with_ai_(false)
{
}

CLIInterface::~CLIInterface()
{
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
    event_thread_ = std::thread([this]()
                                { client_.run(); });

    if (request_game_list_)
    {
        std::optional<ListGamesResponse> game_list_resp = client_.requestGamesList();
        if (!game_list_resp.has_value())
        {
            std::cerr << "Failed to get game list" << std::endl;
            return EXIT_FAILURE;
        }
        else
        {
            printGameList(game_list_resp->games);
            client_.stop();
            return EXIT_SUCCESS;
        }
    }

    // Get player name if not provided
    if (player_name_.empty())
    {
        std::cout << "\nEnter your name: ";
        std::getline(std::cin, player_name_);

        if (player_name_.empty())
            player_name_ = "Player";
    }

    // Send connect request
    std::optional<ConnectResponse> conn_resp = client_.requestConnect(player_name_);
    if (!conn_resp.has_value() || !conn_resp->success)
    {
        std::cerr << "Failed to connect: "
                  << (conn_resp.has_value() ? conn_resp->message : "No response from server") << std::endl;
        return EXIT_FAILURE;
    }

    // Handle join mode
    handleJoinMode();

    // Handle user input
    handleUserInput();

    // Wait for event thread and input thread
    if (event_thread_.joinable())
        event_thread_.join();

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
    std::cout << "Joined game successfully!" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void CLIInterface::joinGame()
{
    std::cout << "Auto-joining first available game..." << std::endl;

    std::optional<JoinGameResponse> join_resp = client_.requestJoinGame(0);
    if (!join_resp.has_value() || !join_resp->success)
    {
        std::cerr << "Failed to join any game: "
                  << (join_resp.has_value() ? join_resp->message : "No response from server") << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void CLIInterface::joinGame(uint32_t game_id)
{
    std::cout << "Joining game ID(" + std::to_string(game_id) + ") ..." << std::endl;

    std::optional<JoinGameResponse> join_resp = client_.requestJoinGame(game_id);
    if (!join_resp.has_value() || !join_resp->success)
    {
        std::cerr << "Failed to join game ID " + std::to_string(game_id) + ": "
                  << (join_resp.has_value() ? join_resp->message : "No response from server") << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void CLIInterface::joinGame(const std::string &name)
{
    std::cout << "Looking for game '" + name + "'..." << std::endl;

    std::optional<ListGamesResponse> game_list_resp = client_.requestGamesList();
    if (!game_list_resp.has_value())
    {
        std::cerr << "Failed to get game list" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Find game by name
    std::optional<uint32_t> target_game;
    for (const auto &game : game_list_resp.value().games)
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
    std::optional<JoinGameResponse> join_resp = client_.requestJoinGame(target_game.value());
    if (!join_resp.has_value() || !join_resp->success)
    {
        std::cerr << "Failed to join game '" + name + "': "
                  << (join_resp.has_value() ? join_resp->message : "No response from server") << std::endl;
        std::exit(EXIT_FAILURE);
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

    std::cout << "Creating game: " << spec.name << "<"
              << (int)spec.rows << ", "
              << (int)spec.cols << ", "
              << (int)spec.num_players << ", "
              << (int)spec.connect_length << "> ..." << std::endl;

    GameConfig config(spec.rows, spec.cols, spec.num_players, spec.connect_length);

    std::optional<CreateGameResponse> create_resp = client_.requestCreateGame(config, spec.name);
    if (!create_resp.has_value() || !create_resp->success)
    {
        std::cerr << "Failed to create game: "
                  << (create_resp.has_value() ? create_resp->message : "No response from server") << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void CLIInterface::setupCallbacks()
{
    // Game state update
    client_.onGameStateUpdate(
        [this](const GameStateUpdate &state)
        {
            waiting_for_input_ = false;
            switch (state.status)
            {

            case ProtocolGameStatus::NOT_STARTED:
                system(CLEAR_COMMAND);
                printLobby(state);
                break;

            case ProtocolGameStatus::IN_PROGRESS:
                if (first_update)
                {
                    first_update = false;
                    printLobby(state);
                    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
                }

                system(CLEAR_COMMAND);
                printBoard(state);

                if (client_.isMyTurn())
                {
                    waiting_for_input_ = true;
                    std::cout << "It's your turn! Make a move." << std::endl;
                }
                else
                {
                    std::cout << "Wait for your turn..." << std::endl;
                }
                break;

            case ProtocolGameStatus::FINISHED_WIN:
                system(CLEAR_COMMAND);
                printBoard(state);
                if (state.winner.value() == client_.getPlayerId())
                {
                    std::cout << "You win :)" << std::endl;
                }
                else
                {
                    std::cout << "Player " + state.player_names[state.winner.value() - 1] + " wins.." << std::endl;
                }
                client_.stop();
                break;

            case ProtocolGameStatus::FINISHED_DRAW:
                system(CLEAR_COMMAND);
                printBoard(state);
                std::cout << "It's a draw.. " << std::endl;
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
                std::cout << std::endl;
                std::cout << message << std::endl;
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
    while (client_.isRunning())
    {
        if (!waiting_for_input_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // TODO: clean the input buffer from previous typed characters during opponent's turn
        int column = -1;
        std::cout << "Enter column (1-" << (int)(client_.getGameInfo()->config.cols) << "): " << std::flush;

        if (!(std::cin >> column))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << std::endl;
            std::cout << "Invalid input. Try again." << std::endl;
            continue;
        }

        if (!client_.sendMove(static_cast<uint8_t>(column - 1)))
        {
            std::cerr << "Failed to send move to server." << std::endl;
        }

        waiting_for_input_ = false;
    }
}
