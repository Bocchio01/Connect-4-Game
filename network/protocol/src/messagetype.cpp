#include "protocol/messagetype.hpp"

std::string messageTypeToString(MessageType type)
{
    switch (type)
    {
    case MessageType::CONNECT_REQUEST:
        return "CONNECT_REQUEST";
    case MessageType::CONNECT_RESPONSE:
        return "CONNECT_RESPONSE";
    case MessageType::DISCONNECT:
        return "DISCONNECT";
    case MessageType::HEARTBEAT:
        return "HEARTBEAT";
    case MessageType::MAKE_MOVE:
        return "MAKE_MOVE";
    case MessageType::MOVE_RESULT:
        return "MOVE_RESULT";
    case MessageType::GAME_STATE_UPDATE:
        return "GAME_STATE_UPDATE";
    case MessageType::GAME_OVER:
        return "GAME_OVER";
    case MessageType::CREATE_GAME:
        return "CREATE_GAME";
    case MessageType::JOIN_GAME:
        return "JOIN_GAME";
    case MessageType::LIST_GAMES:
        return "LIST_GAMES";
    case MessageType::GAME_LIST_RESPONSE:
        return "GAME_LIST_RESPONSE";
    case MessageType::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}