#pragma once

#include <string>
#include <atomic>
#include <thread>

#include "client/client.hpp"

enum class JoinMode
{
    AUTO,          // Join first available game
    BY_ID,         // Join specific game by ID
    BY_NAME,       // Join game by name
    CREATE_CUSTOM, // Create custom game
};

struct GameSpec
{
    std::string name;
    uint8_t rows;
    uint8_t cols;
    uint8_t num_players;
    uint8_t connect_length;
};

/**
 * Command-line interface for Connect X
 * Handles user input/output and game display
 */
class CLIInterface
{
public:
    CLIInterface();
    ~CLIInterface();

    /**
     * Run the CLI application
     * @return Exit code (0 = success)
     */
    int run(int argc, char *argv[]);

private:
    Client client_;
    std::atomic<bool> running_;
    std::atomic<bool> waiting_for_input_;
    std::thread event_thread_;

    // Configuration
    std::string server_host_;
    uint16_t server_port_;
    std::string player_name_;
    JoinMode join_mode_;
    std::optional<uint32_t> target_game_id_;
    std::optional<std::string> target_game_name_;
    std::optional<GameSpec> custom_game_spec_;
    bool request_game_list_;
    bool with_ai_;

    // Setup and initialization
    void parseArguments(int argc, char *argv[]);
    bool parseGameSpec(const std::string &spec);
    void setupCallbacks();
    void handleJoinMode();
    void joinGame();
    void joinGame(uint32_t game_id);
    void joinGame(const std::string &name);
    void createCustomGame();

    // Display functions
    void printBanner();
    void printBoard(const GameStateUpdate &state);
    void printHelp();
    void printGameList(const std::vector<GameInfo> &games);

    // Input handling
    void handleUserInput();

    // Utility
    std::string colorize(const std::string &text, int color_code);
};