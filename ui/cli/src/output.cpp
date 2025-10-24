#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <regex>
#include <limits>
#include <map>

#include "protocol/protocol.hpp"

#include "cli/interface.hpp"
#include "cli/visual.hpp"

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

void CLIInterface::printBanner()
{
    system(CLEAR_COMMAND);
    std::cout << std::string(33, '*') << std::endl;
    std::cout << "*        Connect X Game!        *" << std::endl;
    std::cout << "* TB whises you a great time :) *" << std::endl;
    std::cout << std::string(33, '*') << std::endl;
    std::cout << std::endl;
}

void CLIInterface::printHelp()
{
    std::cout << "Connect X CLI Client" << std::endl
              << std::endl;
    std::cout << "Usage: connectx-cli [options]"
              << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help             Show this help message" << std::endl;
    std::cout << "  -n, --name             Your player name" << std::endl;
    std::cout << "  -g, --game             Game specification (see below)" << std::endl;
    std::cout << "  -l, --list             List available games and exit" << std::endl;
    // std::cout << "  --ai                   Create game with AI opponent" << std::endl;
    std::cout << "  --host                 Server hostname (default: localhost)" << std::endl;
    std::cout << "  -p, --port             Server port (default: " << Protocol::DEFAULT_PORT << ")" << std::endl;
    std::cout << "\nGame Specification Formats:" << std::endl;
    std::cout << "  <game_id>              Join game by ID (e.g., -g 42)" << std::endl;
    std::cout << "  <name>            Join game by name (e.g., -g MyGame)" << std::endl;
    std::cout << "  <name<r c p n>>        Create custom game:" << std::endl;
    std::cout << "                         name: game name" << std::endl;
    std::cout << "                         r: rows, c: columns" << std::endl;
    std::cout << "                         p: num players, n: connect length" << std::endl;
    std::cout << "                         Example: -g 'MyGame<8 10 3 5>'" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  connectx-cli" << std::endl;
    std::cout << "  connectx-cli --name Alice" << std::endl;
    std::cout << "  connectx-cli -g 42" << std::endl;
    std::cout << "  connectx-cli -g MyGame" << std::endl;
    std::cout << "  connectx-cli -g 'BigGame<10 12 3 5>'" << std::endl;
    // std::cout << "  connectx-cli --ai" << std::endl;
}

void CLIInterface::printBoard(const GameStateUpdate &state)
{
    std::cout << "You are playing as: "
              << colorize(std::string(1, static_cast<char>(PLAYER_VISUALS.at(client_.getPlayerId()).symbol)),
                          PLAYER_VISUALS.at(client_.getPlayerId()).color)
              << std::endl;

    std::optional<GameInfo> const gameInfo = client_.getGameInfo();
    if (gameInfo.has_value())
    {
        std::cout << "GameInfo: " << gameInfo->game_name << "<"
                  << (int)gameInfo->config.rows << ", "
                  << (int)gameInfo->config.cols << ", "
                  << (int)gameInfo->config.num_players << ", "
                  << (int)gameInfo->config.connect_length << ">"
                  << std::endl;
    }
    std::cout << std::endl;

    // Rows
    for (uint8_t r = 0; r < state.rows; ++r)
    {
        for (uint8_t c = 0; c < state.cols; ++c)
        {
            uint8_t cell = state.board[r][c];
            PlayerVisual visual = PLAYER_VISUALS.at(cell);
            std::string symbol(1, static_cast<char>(visual.symbol));
            std::cout << " " << colorize(symbol, visual.color) << " ";
        }
        std::cout << std::endl;
    }

    // Bottom border
    std::cout << std::string(3 * state.cols, '-') << std::endl;

    // Column numbers
    for (uint8_t c = 0; c < state.cols; ++c)
    {
        std::cout << " " << std::to_string(c);
        if (c < 10)
        {
            std::cout << " ";
        }
    }
    std::cout << std::endl
              << std::endl;
}

void CLIInterface::printGameList(const std::vector<GameInfo> &games)
{
    if (games.empty())
    {
        std::cout << "\nNo available games.\n";
        return;
    }

    std::cout << "\nAvailable Games:\n";
    std::cout << std::string(55, '=') << "\n";

    // Header
    std::cout << std::left
              << std::setw(6) << "ID"
              << std::setw(25) << "GameInfo"
              << std::setw(10) << "Players"
              << std::setw(14) << "Status"
              << "\n";
    std::cout << std::string(55, '-') << "\n";

    // Rows
    for (const auto &game : games)
    {
        // Format GameInfo (like your preferred style)
        std::ostringstream info;
        info << game.game_name << "<"
             << (int)game.config.rows << ", "
             << (int)game.config.cols << ", "
             << (int)game.config.num_players << ", "
             << (int)game.config.connect_length << ">";

        // Format player count
        std::ostringstream players;
        players << (int)game.current_players << "/"
                << (int)game.config.num_players;

        // Determine status
        std::string status;
        switch (game.status)
        {
        case ProtocolGameStatus::NOT_STARTED:
            status = "Not Started";
            break;
        case ProtocolGameStatus::IN_PROGRESS:
            status = "In Progress";
            break;
        default:
            status = "Finished";
            break;
        }

        // Print row
        std::cout << std::left
                  << std::setw(6) << game.game_id
                  << std::setw(25) << info.str()
                  << std::setw(10) << players.str()
                  << std::setw(14) << status
                  << "\n";
    }

    std::cout << std::string(55, '=') << "\n";
}

std::string CLIInterface::colorize(const std::string &text, int color_code)
{
    std::ostringstream oss;
    oss << "\033[" << color_code << "m" << text << "\033[0m";
    return oss.str();
}
