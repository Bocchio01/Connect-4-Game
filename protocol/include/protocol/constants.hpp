#pragma once

#include <cstdint>

namespace Protocol
{
    // Version
    constexpr uint32_t VERSION = 1;

    // Network
    constexpr uint16_t DEFAULT_PORT = 8080;
    constexpr size_t MAX_MESSAGE_SIZE = 8192;         // 8KB max message
    constexpr uint32_t HEARTBEAT_INTERVAL_MS = 30000; // 30 seconds
    constexpr uint32_t CONNECTION_TIMEOUT_MS = 60000; // 60 seconds

    // Game limits
    constexpr uint8_t MAX_PLAYERS = 4;
    constexpr uint8_t MIN_BOARD_SIZE = 4;
    constexpr uint8_t MAX_BOARD_SIZE = 20;
    constexpr uint8_t MAX_PLAYER_NAME_LENGTH = 32;

    // Error codes
    namespace ErrorCode
    {
        constexpr uint16_t UNKNOWN_ERROR = 0;
        constexpr uint16_t INVALID_MESSAGE = 1;
        constexpr uint16_t PROTOCOL_VERSION_MISMATCH = 2;

        // Connection errors (100-199)
        constexpr uint16_t CONNECTION_REFUSED = 100;
        constexpr uint16_t ALREADY_CONNECTED = 101;
        constexpr uint16_t SESSION_EXPIRED = 102;
        constexpr uint16_t INVALID_SESSION_TOKEN = 103;

        // Game errors (200-299)
        constexpr uint16_t GAME_NOT_FOUND = 200;
        constexpr uint16_t GAME_FULL = 201;
        constexpr uint16_t GAME_ALREADY_STARTED = 202;
        constexpr uint16_t GAME_NOT_STARTED = 203;

        // Move errors (300-399)
        constexpr uint16_t INVALID_MOVE = 300;
        constexpr uint16_t NOT_YOUR_TURN = 301;
        constexpr uint16_t COLUMN_FULL = 302;
        constexpr uint16_t GAME_ALREADY_OVER = 303;
    }
}