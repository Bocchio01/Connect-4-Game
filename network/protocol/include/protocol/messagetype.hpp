#pragma once

#include <cstdint>
#include <string>

#ifdef ERROR
#undef ERROR
#endif

/**
 * All possible message types in the protocol
 */
enum class MessageType : uint8_t
{
    // Connection management
    REQ_CONNECT = 0x01,
    RES_CONNECT = 0x02,
    DISCONNECT = 0x03,
    HEARTBEAT = 0x04,

    // Game actions
    MAKE_MOVE = 0x10,
    MOVE_RESULT = 0x11,
    GAME_STATE_UPDATE = 0x12,

    // Lobby/matchmaking
    REQ_CREATE_GAME = 0x20,
    RES_CREATE_GAME = 0x21,
    REQ_LIST_GAMES = 0x22,
    RES_LIST_GAMES = 0x23,
    REQ_JOIN_GAME = 0x24,
    RES_JOIN_GAME = 0x25,

    // Error handling
    ERROR = 0xFF
};

/**
 * Convert MessageType to string for logging/debugging
 */
std::string messageTypeToString(MessageType type);
