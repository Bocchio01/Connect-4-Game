#include <iostream>
#include <cstring>
#include <spdlog/spdlog.h>

#include "client/client.hpp"

Client::Client()
    : connected_(false),
      running_(false),
      player_id_(0)
{
}

Client::~Client()
{
    disconnect();
}

// ============================================================================
// Connection Management
// ============================================================================

bool Client::connect(const std::string &host, uint16_t port)
{
    spdlog::info("Connecting to {}:{}...", host, port);

    if (connected_)
    {
        spdlog::warn("Already connected");
        return false;
    }

    // Initialize sockpp
    sockpp::socket_initializer::initialize();

    // Connect
    if (!socket_.connect(sockpp::inet_address(host, port)))
    {
        spdlog::error("Connection failed: {}", socket_.last_error_str());
        return false;
    }

    connected_ = true;
    spdlog::info("Connected to the server");

    return true;
}

void Client::disconnect()
{
    spdlog::info("Disconnecting from server...");

    if (!connected_)
    {
        spdlog::warn("Not connected");
        return;
    }

    running_ = false;
    connected_ = false;

    if (socket_.is_open())
    {
        socket_.close();
    }

    if (disconnected_callback_)
    {
        disconnected_callback_();
    }

    spdlog::info("Disconnected");
}

void Client::run()
{
    spdlog::debug("Starting event loop...");

    if (!connected_)
    {
        spdlog::error("Cannot run event loop: not connected");
        return;
    }

    running_ = true;

    while (running_ && connected_)
    {
        try
        {
            std::string message = readMessage();

            if (message.empty())
            {
                // Connection closed or error
                spdlog::info("Connection closed by server");
                disconnect();
                break;
            }

            processMessage(message);
        }
        catch (const std::exception &e)
        {
            spdlog::error("Error in event loop: {}", e.what());
            disconnect();
            break;
        }
    }

    spdlog::debug("Event loop stopped");
}

void Client::stop()
{
    running_ = false;
    if (connected_)
    {
        disconnect();
    }
}

// ============================================================================
// Game Actions
// ============================================================================

bool Client::sendConnectRequest(const std::string &player_name)
{
    spdlog::debug("Sending connect request as '{}'", player_name);

    ConnectRequest req(player_name);
    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::REQ_CONNECT, payload);
}

bool Client::sendMove(uint8_t column)
{
    spdlog::debug("Sending move request: column {}", (int)column);

    MakeMoveRequest req(session_token_, column);
    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::MAKE_MOVE, payload);
}

bool Client::sendCreateGame(const GameConfig &config, const std::string &name)
{
    spdlog::debug("Sending create game request: ({})<{}, {}, {}, {}>",
                  name.empty() ? "default" : name,
                  (int)config.rows, (int)config.cols,
                  (int)config.num_players, (int)config.connect_length);

    CreateGameRequest req;
    req.session_token = session_token_;
    req.game_name = name;
    req.config = config;

    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::REQ_CREATE_GAME, payload);
}

bool Client::sendJoinGame(uint32_t game_id)
{
    spdlog::debug("Sending join game request: game ID {}", game_id);

    JoinGameRequest req(session_token_, game_id);
    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::REQ_JOIN_GAME, payload);
}

bool Client::sendListGames()
{
    spdlog::debug("Sending list games request");

    ListGamesRequest req(session_token_);
    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::REQ_LIST_GAMES, payload);
}

bool Client::sendDisconnect(const std::string &reason)
{
    spdlog::debug("Sending disconnect message: {}", reason);

    DisconnectMessage msg(reason);
    std::string payload = MessageSerializer::serialize(msg);
    return sendMessage(MessageType::DISCONNECT, payload);
}

// ===========================================================================
// Synchronous Requests
// ===========================================================================
std::optional<ConnectResponse> Client::requestConnect(const std::string &player_name, int timeout_ms)
{
    sendConnectRequest(player_name);
    auto response = waitForResponse(MessageType::RES_CONNECT, std::chrono::milliseconds(timeout_ms));
    if (!response)
        return std::nullopt;

    ConnectResponse resp = MessageSerializer::deserializeConnectResponse(*response);
    if (resp.success)
    {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            session_token_ = resp.session_token;
        }
        spdlog::debug("Connected successfully, session token: {}", session_token_);
    }
    else
        spdlog::error("Connection failed: {}", resp.message);

    return resp;
}

std::optional<CreateGameResponse> Client::requestCreateGame(const GameConfig &config, const std::string &name, int timeout_ms)
{
    sendCreateGame(config, name);
    auto response = waitForResponse(MessageType::RES_CREATE_GAME, std::chrono::milliseconds(timeout_ms));
    if (!response)
        return std::nullopt;

    CreateGameResponse resp = MessageSerializer::deserializeCreateGameResponse(*response);
    if (resp.success)
    {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            game_info_ = resp.game_info;
        }
        spdlog::debug("Game created successfully, game ID: {}", resp.game_info.game_id);
    }
    else
        spdlog::warn("Create game failed: {}", resp.message);

    return resp;
}

std::optional<ListGamesResponse> Client::requestGamesList(int timeout_ms)
{
    sendListGames();
    auto response = waitForResponse(MessageType::RES_LIST_GAMES, std::chrono::milliseconds(timeout_ms));
    if (!response)
        return std::nullopt;

    ListGamesResponse resp = MessageSerializer::deserializeGameListResponse(*response);
    if (resp.games.size() > 0)
        spdlog::debug("Received game list response: {} games available", resp.games.size());
    else
        spdlog::warn("List games returned no available games");

    return resp;
}

std::optional<JoinGameResponse> Client::requestJoinGame(uint32_t game_id, int timeout_ms)
{
    sendJoinGame(game_id);
    auto response = waitForResponse(MessageType::RES_JOIN_GAME, std::chrono::milliseconds(timeout_ms));
    if (!response)
        return std::nullopt;

    JoinGameResponse resp = MessageSerializer::deserializeJoinGameResponse(*response);
    if (resp.success)
    {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            game_info_ = resp.game_info;
            player_id_ = resp.assigned_player_id;
        }
        spdlog::debug("Joined game successfully, game ID: {}, assigned player ID: {}",
                      resp.game_info.game_id, resp.assigned_player_id);
    }
    else
        spdlog::warn("Join game failed: {}", resp.message);

    return resp;
}

// ============================================================================
// Internal Methods
// ============================================================================

bool Client::sendMessage(MessageType type, const std::string &payload)
{
    if (!connected_)
    {
        spdlog::error("Cannot send message: not connected");
        return false;
    }

    std::string message = MessageSerializer::wrapMessage(type, payload);
    return writeMessage(message);
}

std::optional<std::string> Client::waitForResponse(MessageType type, std::chrono::milliseconds timeout)
{
    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();

    {
        std::lock_guard<std::mutex> lock(promise_mutex_);
        pending_promises_[type] = std::move(promise);
    }

    if (future.wait_for(timeout) == std::future_status::ready)
    {
        return future.get();
    }

    std::lock_guard<std::mutex> lock(promise_mutex_);
    pending_promises_.erase(type);
    return std::nullopt;
}

std::string Client::readMessage()
{
    // Read 4-byte length prefix
    uint32_t length = 0;
    ssize_t n = socket_.read_n(&length, sizeof(length));

    if (n != sizeof(length))
    {
        return ""; // Connection closed or error
    }

    // Convert from network byte order
    length = ntohl(length);

    // Validate message size
    if (length == 0 || length > Protocol::MAX_MESSAGE_SIZE)
    {
        spdlog::error("Invalid message length: {}", length);
        return "";
    }

    // Read message data
    std::vector<char> buffer(length);
    n = socket_.read_n(buffer.data(), length);

    if (n != static_cast<ssize_t>(length))
    {
        return ""; // Connection closed or error
    }

    return std::string(buffer.begin(), buffer.end());
}

bool Client::writeMessage(const std::string &message)
{
    // Validate message size
    if (message.size() > Protocol::MAX_MESSAGE_SIZE)
    {
        spdlog::error("Message too large: {}", message.size());
        return false;
    }

    // Write 4-byte length prefix
    uint32_t length = htonl(static_cast<uint32_t>(message.size()));
    ssize_t n = socket_.write_n(&length, sizeof(length));

    if (n != sizeof(length))
    {
        return false;
    }

    // Write message data
    n = socket_.write_n(message.data(), message.size());

    return n == static_cast<ssize_t>(message.size());
}

void Client::processMessage(const std::string &data)
{
    try
    {
        // Unwrap message
        auto [type, payload] = MessageSerializer::unwrapMessage(data);

        spdlog::debug("Received message of type {}", messageTypeToString(type));

        // Check for pending promise
        {
            std::lock_guard<std::mutex> lock(promise_mutex_);
            auto it = pending_promises_.find(type);
            if (it != pending_promises_.end())
            {
                it->second.set_value(payload);
                pending_promises_.erase(it);
                return; // handled via promise
            }
        }

        // Route to appropriate handler
        switch (type)
        {
        case MessageType::GAME_STATE_UPDATE:
            handleGameStateUpdate(payload);
            break;

        case MessageType::MOVE_RESULT:
            handleMoveResult(payload);
            break;

        case MessageType::ERROR:
            handleError(payload);
            break;

        default:
            spdlog::warn("Unknown message type: {}", static_cast<int>(type));
            break;
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error processing message: {}", e.what());
    }
}

// ============================================================================
// Message Handlers
// ============================================================================

void Client::handleGameStateUpdate(const std::string &payload)
{
    GameStateUpdate update = MessageSerializer::deserializeGameStateUpdate(payload);

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        current_state_ = update;
    }

    spdlog::debug("Received game state update for game ID: {}", update.game_id);

    if (game_state_callback_)
    {
        game_state_callback_(update);
    }
}

void Client::handleMoveResult(const std::string &payload)
{
    MoveResult result = MessageSerializer::deserializeMoveResult(payload);

    spdlog::debug("Received move result: success={}, message='{}'",
                  result.success, result.message);

    if (move_result_callback_)
    {
        move_result_callback_(result.success, result.message);
    }
}

void Client::handleError(const std::string &payload)
{
    ErrorMessage err = MessageSerializer::deserializeError(payload);

    spdlog::error("Server error {}: {}", err.error_code, err.error_message);

    if (error_callback_)
    {
        error_callback_(err.error_code, err.error_message);
    }
}