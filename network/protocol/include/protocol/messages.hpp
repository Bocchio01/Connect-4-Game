#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

/**
 * Game status (mirrors GameStatus from core but for protocol)
 */
enum class ProtocolGameStatus : uint8_t
{
    NOT_STARTED = 0,
    IN_PROGRESS = 1,
    FINISHED_WIN = 2,
    FINISHED_DRAW = 3
};

// ============================================================================
// Connection Messages
// ============================================================================

struct ConnectRequest
{
    uint32_t protocol_version;
    std::string player_name;

    ConnectRequest() : protocol_version(Protocol::VERSION) {}
    explicit ConnectRequest(const std::string &name)
        : protocol_version(Protocol::VERSION), player_name(name) {}
};

struct ConnectResponse
{
    bool success;
    std::string session_token;
    std::string message; // Success or error message

    ConnectResponse() : success(false) {}
};

struct DisconnectMessage
{
    std::string reason;

    DisconnectMessage() = default;
    explicit DisconnectMessage(const std::string &r) : reason(r) {}
};

struct HeartbeatMessage
{
    uint64_t timestamp;

    HeartbeatMessage() : timestamp(0) {}
    explicit HeartbeatMessage(uint64_t ts) : timestamp(ts) {}
};

// ============================================================================
// Game Action Messages
// ============================================================================

struct MakeMoveRequest
{
    std::string session_token;
    uint8_t column;

    MakeMoveRequest() : column(0) {}
    MakeMoveRequest(const std::string &token, uint8_t col)
        : session_token(token), column(col) {}
};

struct MoveResult
{
    bool success;
    std::string message; // Error message if failed

    MoveResult() : success(false) {}
    explicit MoveResult(bool s, const std::string &msg = "")
        : success(s), message(msg) {}
};

struct GameStateUpdate
{
    uint32_t game_id;
    std::vector<std::vector<uint8_t>> board; // 2D grid
    uint8_t rows;
    uint8_t cols;
    uint8_t current_player;
    ProtocolGameStatus status;
    std::optional<uint8_t> winner;
    std::vector<uint8_t> players;          // List of player IDs in game
    std::vector<std::string> player_names; // List of player names in game

    GameStateUpdate() : game_id(0), rows(0), cols(0),
                        current_player(0), status(ProtocolGameStatus::NOT_STARTED) {}
};

struct GameOverMessage
{
    uint32_t game_id;
    ProtocolGameStatus final_status;
    std::optional<uint8_t> winner;
    std::string message;

    GameOverMessage() : game_id(0), final_status(ProtocolGameStatus::FINISHED_DRAW) {}
};

// ============================================================================
// Lobby Messages
// ============================================================================

struct GameConfig
{
    uint8_t rows;
    uint8_t cols;
    uint8_t num_players;
    uint8_t connect_length;

    GameConfig() : rows(6), cols(7), num_players(2), connect_length(4) {}
    GameConfig(uint8_t r, uint8_t c, uint8_t np, uint8_t cl)
        : rows(r), cols(c), num_players(np), connect_length(cl) {}
};

struct GameInfo
{
    uint32_t game_id;
    std::string game_name;
    uint8_t current_players;
    ProtocolGameStatus status;
    GameConfig config;

    GameInfo() : game_id(0), current_players(0),
                 status(ProtocolGameStatus::NOT_STARTED) {}
};

struct CreateGameRequest
{
    std::string session_token;
    std::string game_name;
    GameConfig config;

    CreateGameRequest() = default;
};

struct CreateGameResponse
{
    bool success;
    GameInfo game_info;
    std::string message; // Success or error message

    CreateGameResponse() : success(false) {}
};

struct JoinGameRequest
{
    std::string session_token;
    uint32_t game_id;

    JoinGameRequest() : game_id(0) {}
    JoinGameRequest(const std::string &token, uint32_t gid)
        : session_token(token), game_id(gid) {}
};

struct JoinGameResponse
{
    bool success;
    uint8_t assigned_player_id;
    GameInfo game_info;
    std::string message; // Success or error message

    JoinGameResponse() : success(false), assigned_player_id(0) {}
};

struct ListGamesRequest
{
    std::string session_token;

    ListGamesRequest() = default;
    explicit ListGamesRequest(const std::string &token) : session_token(token) {}
};

struct ListGamesResponse
{
    std::vector<GameInfo> games;

    ListGamesResponse() = default;
};

// ============================================================================
// Error Message
// ============================================================================

struct ErrorMessage
{
    uint16_t error_code;
    std::string error_message;

    ErrorMessage() : error_code(Protocol::ErrorCode::UNKNOWN_ERROR) {}
    ErrorMessage(uint16_t code, const std::string &msg)
        : error_code(code), error_message(msg) {}
};