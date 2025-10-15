#pragma once

#include <string>
#include <atomic>
#include <thread>

#include "client/client.hpp"

/**
 * Command-line interface for Connect 4
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

    // Setup and initialization
    void parseArguments(int argc, char *argv[]);
    void printBanner();
    void setupCallbacks();
    bool connectToServer();

    // Display functions
    void printBoard(const GameStateUpdate &state);
    void printHelp();
    void clearScreen();
    void printSeparator(char ch = '=', int length = 50);

    // Input handling
    void handleUserInput();
    int getColumnInput();

    // Utility
    std::string getPlayerSymbol(uint8_t player_id);
    std::string colorize(const std::string &text, int color_code);
};