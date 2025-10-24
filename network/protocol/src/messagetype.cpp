#include "protocol/messagetype.hpp"

std::string messageTypeToString(MessageType type)
{
    switch (type)
    {
    case MessageType::REQ_CONNECT:
        return "REQ_CONNECT";
    case MessageType::RES_CONNECT:
        return "RES_CONNECT";
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
    case MessageType::REQ_CREATE_GAME:
        return "CREATE_GAME";
    case MessageType::REQ_JOIN_GAME:
        return "REQ_JOIN_GAME";
    case MessageType::REQ_LIST_GAMES:
        return "REQ_LIST_GAMES";
    case MessageType::RES_LIST_GAMES:
        return "RES_LIST_GAMES";
    case MessageType::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}