#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <regex>
#include <limits>
#include <map>

#include "cli/interface.hpp"

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
        else if (arg == "-l" || arg == "--list")
        {
            request_game_list_ = true;
        }
        else if (arg == "--host" && i + 1 < argc)
        {
            server_host_ = argv[++i];
        }
        else if ((arg == "-p" || arg == "--port") && i + 1 < argc)
        {
            server_port_ = static_cast<uint16_t>(std::atoi(argv[++i]));
        }
        else if ((arg == "-n" || arg == "--name") && i + 1 < argc)
        {
            player_name_ = argv[++i];
        }
        else if ((arg == "-g" || arg == "--game") && i + 1 < argc)
        {
            std::string game_arg = argv[++i];

            // Check if it's a game specification (contains '<')
            if (game_arg.find('<') != std::string::npos)
            {
                if (parseGameSpec(game_arg))
                {
                    join_mode_ = JoinMode::CREATE_CUSTOM;
                }
                else
                {
                    std::cerr << "Invalid game specification format" << std::endl;
                    exit(1);
                }
            }
            else
            {
                // Try to parse as game ID (number)
                try
                {
                    uint32_t game_id = std::stoul(game_arg);
                    join_mode_ = JoinMode::BY_ID;
                    target_game_id_ = game_id;
                }
                catch (...)
                {
                    // It's a game name
                    join_mode_ = JoinMode::BY_NAME;
                    target_game_name_ = game_arg;
                }
            }
        }
    }
}

bool CLIInterface::parseGameSpec(const std::string &spec)
{
    // Format: GameName<rows cols num_players connect_length>
    // Example: MyGame<8 10 3 5>

    std::regex pattern(R"(([^<]*)<(\d+)\s+(\d+)\s+(\d+)\s+(\d+)>)");
    std::smatch match;

    if (std::regex_match(spec, match, pattern))
    {
        GameSpec gs;
        gs.name = match[1];
        gs.rows = static_cast<uint8_t>(std::stoi(match[2]));
        gs.cols = static_cast<uint8_t>(std::stoi(match[3]));
        gs.num_players = static_cast<uint8_t>(std::stoi(match[4]));
        gs.connect_length = static_cast<uint8_t>(std::stoi(match[5]));

        custom_game_spec_ = gs;
        return true;
    }

    return false;
}
