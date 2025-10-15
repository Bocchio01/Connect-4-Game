#include <stdexcept>
#include <nlohmann/json.hpp>
#include "protocol/constants.hpp"
#include "protocol/serializer.hpp"

using json = nlohmann::json;

// ============================================================================
// Helper functions for optional types
// ============================================================================

namespace
{
    template <typename T>
    json optionalToJson(const std::optional<T> &opt)
    {
        if (opt.has_value())
        {
            return opt.value();
        }
        return nullptr;
    }

    template <typename T>
    std::optional<T> jsonToOptional(const json &j)
    {
        if (j.is_null())
        {
            return std::nullopt;
        }
        return j.get<T>();
    }
}

// ============================================================================
// Connection Messages
// ============================================================================

std::string MessageSerializer::serialize(const ConnectRequest &msg)
{
    json j;
    j["protocol_version"] = msg.protocol_version;
    j["player_name"] = msg.player_name;
    return j.dump();
}

ConnectRequest MessageSerializer::deserializeConnectRequest(const std::string &json_str)
{
    json j = json::parse(json_str);
    ConnectRequest msg;
    msg.protocol_version = j["protocol_version"];
    msg.player_name = j["player_name"];
    return msg;
}

std::string MessageSerializer::serialize(const ConnectResponse &msg)
{
    json j;
    j["success"] = msg.success;
    j["session_token"] = msg.session_token;
    j["message"] = msg.message;
    return j.dump();
}

ConnectResponse MessageSerializer::deserializeConnectResponse(const std::string &json_str)
{
    json j = json::parse(json_str);
    ConnectResponse msg;
    msg.success = j["success"];
    msg.session_token = j["session_token"];
    msg.message = j["message"];
    return msg;
}

std::string MessageSerializer::serialize(const DisconnectMessage &msg)
{
    json j;
    j["reason"] = msg.reason;
    return j.dump();
}

DisconnectMessage MessageSerializer::deserializeDisconnect(const std::string &json_str)
{
    json j = json::parse(json_str);
    DisconnectMessage msg;
    msg.reason = j["reason"];
    return msg;
}

std::string MessageSerializer::serialize(const HeartbeatMessage &msg)
{
    json j;
    j["timestamp"] = msg.timestamp;
    return j.dump();
}

HeartbeatMessage MessageSerializer::deserializeHeartbeat(const std::string &json_str)
{
    json j = json::parse(json_str);
    HeartbeatMessage msg;
    msg.timestamp = j["timestamp"];
    return msg;
}

// ============================================================================
// Game Action Messages
// ============================================================================

std::string MessageSerializer::serialize(const MakeMoveRequest &msg)
{
    json j;
    j["session_token"] = msg.session_token;
    j["column"] = msg.column;
    return j.dump();
}

MakeMoveRequest MessageSerializer::deserializeMakeMoveRequest(const std::string &json_str)
{
    json j = json::parse(json_str);
    MakeMoveRequest msg;
    msg.session_token = j["session_token"];
    msg.column = j["column"];
    return msg;
}

std::string MessageSerializer::serialize(const MoveResult &msg)
{
    json j;
    j["success"] = msg.success;
    j["message"] = msg.message;
    return j.dump();
}

MoveResult MessageSerializer::deserializeMoveResult(const std::string &json_str)
{
    json j = json::parse(json_str);
    MoveResult msg;
    msg.success = j["success"];
    msg.message = j["message"];
    return msg;
}

std::string MessageSerializer::serialize(const GameStateUpdate &msg)
{
    json j;
    j["game_id"] = msg.game_id;
    j["board"] = msg.board;
    j["rows"] = msg.rows;
    j["cols"] = msg.cols;
    j["current_player"] = msg.current_player;
    j["status"] = static_cast<uint8_t>(msg.status);
    j["winner"] = optionalToJson(msg.winner);
    j["players"] = msg.players;
    j["player_names"] = msg.player_names;
    return j.dump();
}

GameStateUpdate MessageSerializer::deserializeGameStateUpdate(const std::string &json_str)
{
    json j = json::parse(json_str);
    GameStateUpdate msg;
    msg.game_id = j["game_id"];
    msg.board = j["board"].get<std::vector<std::vector<uint8_t>>>();
    msg.rows = j["rows"];
    msg.cols = j["cols"];
    msg.current_player = j["current_player"];
    msg.status = static_cast<ProtocolGameStatus>(j["status"].get<uint8_t>());
    msg.winner = jsonToOptional<uint8_t>(j["winner"]);
    msg.players = j["players"].get<std::vector<uint8_t>>();
    msg.player_names = j["player_names"].get<std::vector<std::string>>();
    return msg;
}

std::string MessageSerializer::serialize(const GameOverMessage &msg)
{
    json j;
    j["game_id"] = msg.game_id;
    j["final_status"] = static_cast<uint8_t>(msg.final_status);
    j["winner"] = optionalToJson(msg.winner);
    j["message"] = msg.message;
    return j.dump();
}

GameOverMessage MessageSerializer::deserializeGameOver(const std::string &json_str)
{
    json j = json::parse(json_str);
    GameOverMessage msg;
    msg.game_id = j["game_id"];
    msg.final_status = static_cast<ProtocolGameStatus>(j["final_status"].get<uint8_t>());
    msg.winner = jsonToOptional<uint8_t>(j["winner"]);
    msg.message = j["message"];
    return msg;
}

// ============================================================================
// Lobby Messages
// ============================================================================

std::string MessageSerializer::serialize(const CreateGameRequest &msg)
{
    json j;
    j["session_token"] = msg.session_token;
    j["name"] = msg.game_name;
    j["config"]["rows"] = msg.config.rows;
    j["config"]["cols"] = msg.config.cols;
    j["config"]["num_players"] = msg.config.num_players;
    j["config"]["connect_length"] = msg.config.connect_length;
    return j.dump();
}

CreateGameRequest MessageSerializer::deserializeCreateGameRequest(const std::string &json_str)
{
    json j = json::parse(json_str);
    CreateGameRequest msg;
    msg.session_token = j["session_token"];
    msg.game_name = j["name"];
    msg.config.rows = j["config"]["rows"];
    msg.config.cols = j["config"]["cols"];
    msg.config.num_players = j["config"]["num_players"];
    msg.config.connect_length = j["config"]["connect_length"];
    return msg;
}

std::string MessageSerializer::serialize(const CreateGameResponse &msg)
{
    json j;
    j["success"] = msg.success;
    j["assigned_player_id"] = msg.assigned_player_id;
    j["game_info"]["game_id"] = msg.game_info.game_id;
    j["game_info"]["name"] = msg.game_info.game_name;
    j["game_info"]["current_players"] = msg.game_info.current_players;
    j["game_info"]["status"] = static_cast<uint8_t>(msg.game_info.status);
    j["game_info"]["config"]["rows"] = msg.game_info.config.rows;
    j["game_info"]["config"]["cols"] = msg.game_info.config.cols;
    j["game_info"]["config"]["num_players"] = msg.game_info.config.num_players;
    j["game_info"]["config"]["connect_length"] = msg.game_info.config.connect_length;
    j["message"] = msg.message;
    return j.dump();
}

CreateGameResponse MessageSerializer::deserializeCreateGameResponse(const std::string &json_str)
{
    json j = json::parse(json_str);
    CreateGameResponse msg;
    msg.success = j["success"];
    msg.assigned_player_id = j["assigned_player_id"];
    msg.game_info.game_id = j["game_info"]["game_id"];
    msg.game_info.game_name = j["game_info"]["name"];
    msg.game_info.current_players = j["game_info"]["current_players"];
    msg.game_info.status = static_cast<ProtocolGameStatus>(j["game_info"]["status"].get<uint8_t>());
    msg.game_info.config.rows = j["game_info"]["config"]["rows"];
    msg.game_info.config.cols = j["game_info"]["config"]["cols"];
    msg.game_info.config.num_players = j["game_info"]["config"]["num_players"];
    msg.game_info.config.connect_length = j["game_info"]["config"]["connect_length"];
    msg.message = j["message"];
    return msg;
}

std::string MessageSerializer::serialize(const JoinGameRequest &msg)
{
    json j;
    j["session_token"] = msg.session_token;
    j["game_id"] = msg.game_id;
    return j.dump();
}

JoinGameRequest MessageSerializer::deserializeJoinGameRequest(const std::string &json_str)
{
    json j = json::parse(json_str);
    JoinGameRequest msg;
    msg.session_token = j["session_token"];
    msg.game_id = j["game_id"];
    return msg;
}

std::string MessageSerializer::serialize(const JoinGameResponse &msg)
{
    json j;
    j["success"] = msg.success;
    j["assigned_player_id"] = msg.assigned_player_id;
    j["game_info"]["game_id"] = msg.game_info.game_id;
    j["game_info"]["name"] = msg.game_info.game_name;
    j["game_info"]["current_players"] = msg.game_info.current_players;
    j["game_info"]["status"] = static_cast<uint8_t>(msg.game_info.status);
    j["game_info"]["config"]["rows"] = msg.game_info.config.rows;
    j["game_info"]["config"]["cols"] = msg.game_info.config.cols;
    j["game_info"]["config"]["num_players"] = msg.game_info.config.num_players;
    j["game_info"]["config"]["connect_length"] = msg.game_info.config.connect_length;
    j["message"] = msg.message;
    return j.dump();
}

JoinGameResponse MessageSerializer::deserializeJoinGameResponse(const std::string &json_str)
{
    json j = json::parse(json_str);
    JoinGameResponse msg;
    msg.success = j["success"];
    msg.assigned_player_id = j["assigned_player_id"];
    msg.game_info.game_id = j["game_info"]["game_id"];
    msg.game_info.game_name = j["game_info"]["name"];
    msg.game_info.current_players = j["game_info"]["current_players"];
    msg.game_info.status = static_cast<ProtocolGameStatus>(j["game_info"]["status"].get<uint8_t>());
    msg.game_info.config.rows = j["game_info"]["config"]["rows"];
    msg.game_info.config.cols = j["game_info"]["config"]["cols"];
    msg.game_info.config.num_players = j["game_info"]["config"]["num_players"];
    msg.game_info.config.connect_length = j["game_info"]["config"]["connect_length"];
    msg.message = j["message"];
    return msg;
}

std::string MessageSerializer::serialize(const ListGamesRequest &msg)
{
    json j;
    j["session_token"] = msg.session_token;
    return j.dump();
}

ListGamesRequest MessageSerializer::deserializeListGamesRequest(const std::string &json_str)
{
    json j = json::parse(json_str);
    ListGamesRequest msg;
    msg.session_token = j["session_token"];
    return msg;
}

std::string MessageSerializer::serialize(const ListGamesResponse &msg)
{
    json j;
    j["games"] = json::array();

    for (const auto &game : msg.games)
    {
        json game_json;
        game_json["game_id"] = game.game_id;
        game_json["current_players"] = game.current_players;
        game_json["status"] = static_cast<uint8_t>(game.status);
        game_json["config"]["rows"] = game.config.rows;
        game_json["config"]["cols"] = game.config.cols;
        game_json["config"]["num_players"] = game.config.num_players;
        game_json["config"]["connect_length"] = game.config.connect_length;
        j["games"].push_back(game_json);
    }

    return j.dump();
}

ListGamesResponse MessageSerializer::deserializeGameListResponse(const std::string &json_str)
{
    json j = json::parse(json_str);
    ListGamesResponse msg;

    for (const auto &game_json : j["games"])
    {
        GameInfo game;
        game.game_id = game_json["game_id"];
        game.current_players = game_json["current_players"];
        game.status = static_cast<ProtocolGameStatus>(game_json["status"].get<uint8_t>());
        game.config.rows = game_json["config"]["rows"];
        game.config.cols = game_json["config"]["cols"];
        game.config.num_players = game_json["config"]["num_players"];
        game.config.connect_length = game_json["config"]["connect_length"];
        msg.games.push_back(game);
    }

    return msg;
}

// ============================================================================
// Error Message
// ============================================================================

std::string MessageSerializer::serialize(const ErrorMessage &msg)
{
    json j;
    j["error_code"] = msg.error_code;
    j["error_message"] = msg.error_message;
    return j.dump();
}

ErrorMessage MessageSerializer::deserializeError(const std::string &json_str)
{
    json j = json::parse(json_str);
    ErrorMessage msg;
    msg.error_code = j["error_code"];
    msg.error_message = j["error_message"];
    return msg;
}

// ============================================================================
// Generic wrapper functions
// ============================================================================

std::string MessageSerializer::wrapMessage(MessageType type, const std::string &payload)
{
    json j;
    j["type"] = static_cast<uint8_t>(type);
    j["payload"] = json::parse(payload);
    return j.dump();
}

std::pair<MessageType, std::string> MessageSerializer::unwrapMessage(const std::string &json_str)
{
    json j = json::parse(json_str);

    MessageType type = static_cast<MessageType>(j["type"].get<uint8_t>());
    std::string payload = j["payload"].dump();

    return {type, payload};
}

// ============================================================================
// Validation functions
// ============================================================================

bool MessageSerializer::isValidJson(const std::string &json_str)
{
    try
    {
        [[maybe_unused]] auto j = json::parse(json_str);
        return true;
    }
    catch (const json::parse_error &)
    {
        return false;
    }
}

bool MessageSerializer::isValidMessageSize(const std::string &json_str)
{
    return json_str.size() <= Protocol::MAX_MESSAGE_SIZE;
}