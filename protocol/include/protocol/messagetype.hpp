#pragma once

#include <cstdint>
#include <string>

/**
 * All possible message types in the protocol
 */
enum class MessageType : uint8_t
{
    // Connection management
    CONNECT_REQUEST = 0x01,
    CONNECT_RESPONSE = 0x02,
    DISCONNECT = 0x03,
    HEARTBEAT = 0x04,

    // Game actions
    MAKE_MOVE = 0x10,
    MOVE_RESULT = 0x11,
    GAME_STATE_UPDATE = 0x12,
    GAME_OVER = 0x13,

    // Lobby/matchmaking
    CREATE_GAME = 0x20,
    JOIN_GAME = 0x21,
    LIST_GAMES = 0x22,
    GAME_LIST_RESPONSE = 0x23,

    // Error handling
    ERROR = 0xFF
};

/**
 * Convert MessageType to string for logging/debugging
 */
std::string messageTypeToString(MessageType type);
