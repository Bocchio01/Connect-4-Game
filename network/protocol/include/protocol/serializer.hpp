#pragma once

#include <string>
#include <vector>
#include <memory>
#include "protocol/messages.hpp"
#include "protocol/messagetype.hpp"

/**
 * Handles serialization and deserialization of protocol messages
 * Uses JSON format for human readability and ease of debugging
 */
class MessageSerializer
{
public:
    // ========================================================================
    // Serialization (Message → JSON string)
    // ========================================================================

    static std::string serialize(const ConnectRequest &msg);
    static std::string serialize(const ConnectResponse &msg);
    static std::string serialize(const DisconnectMessage &msg);
    static std::string serialize(const HeartbeatMessage &msg);

    static std::string serialize(const MakeMoveRequest &msg);
    static std::string serialize(const MoveResult &msg);
    static std::string serialize(const GameStateUpdate &msg);
    static std::string serialize(const GameOverMessage &msg);

    static std::string serialize(const CreateGameRequest &msg);
    static std::string serialize(const CreateGameResponse &msg);
    static std::string serialize(const JoinGameRequest &msg);
    static std::string serialize(const JoinGameResponse &msg);
    static std::string serialize(const ListGamesRequest &msg);
    static std::string serialize(const ListGamesResponse &msg);

    static std::string serialize(const ErrorMessage &msg);

    // ========================================================================
    // Deserialization (JSON string → Message)
    // ========================================================================

    static ConnectRequest deserializeConnectRequest(const std::string &json);
    static ConnectResponse deserializeConnectResponse(const std::string &json);
    static DisconnectMessage deserializeDisconnect(const std::string &json);
    static HeartbeatMessage deserializeHeartbeat(const std::string &json);

    static MakeMoveRequest deserializeMakeMoveRequest(const std::string &json);
    static MoveResult deserializeMoveResult(const std::string &json);
    static GameStateUpdate deserializeGameStateUpdate(const std::string &json);
    static GameOverMessage deserializeGameOver(const std::string &json);

    static CreateGameRequest deserializeCreateGameRequest(const std::string &json);
    static CreateGameResponse deserializeCreateGameResponse(const std::string &json);
    static JoinGameRequest deserializeJoinGameRequest(const std::string &json);
    static JoinGameResponse deserializeJoinGameResponse(const std::string &json);
    static ListGamesRequest deserializeListGamesRequest(const std::string &json);
    static ListGamesResponse deserializeGameListResponse(const std::string &json);

    static ErrorMessage deserializeError(const std::string &json);

    // ========================================================================
    // Generic wrapper for type-tagged messages
    // ========================================================================

    /**
     * Converts a &lt;MessageType&gt; into a &lt;message_json&gt; object.
     */
    static std::string wrapMessage(MessageType type, const std::string &payload);

    /**
     * Unwraps a type-tagged message
     * Returns: {type, payload_json}
     */
    static std::pair<MessageType, std::string> unwrapMessage(const std::string &json);

    // ========================================================================
    // Validation
    // ========================================================================

    /**
     * Check if a JSON string is valid
     */
    static bool isValidJson(const std::string &json);

    /**
     * Validate message size
     */
    static bool isValidMessageSize(const std::string &json);
};
