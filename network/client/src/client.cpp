#include <iostream>
#include <cstring>

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
    if (connected_)
    {
        std::cerr << "[Client] Already connected" << std::endl;
        return false;
    }

    std::cout << "[Client] Connecting to " << host << ":" << port << "..." << std::endl;

    // Initialize sockpp
    sockpp::socket_initializer::initialize();

    // Connect
    if (!socket_.connect(sockpp::inet_address(host, port)))
    {
        std::cerr << "[Client] Connection failed: " << socket_.last_error_str() << std::endl;
        return false;
    }

    connected_ = true;
    std::cout << "[Client] Connected successfully!" << std::endl;

    return true;
}

void Client::disconnect()
{
    if (!connected_)
    {
        return;
    }

    std::cout << "[Client] Disconnecting..." << std::endl;

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

    std::cout << "[Client] Disconnected" << std::endl;
}

void Client::run()
{
    if (!connected_)
    {
        std::cerr << "[Client] Not connected" << std::endl;
        return;
    }

    running_ = true;
    std::cout << "[Client] Starting event loop..." << std::endl;

    while (running_ && connected_)
    {
        try
        {
            std::string message = readMessage();

            if (message.empty())
            {
                // Connection closed or error
                std::cout << "[Client] Connection closed by server" << std::endl;
                disconnect();
                break;
            }

            processMessage(message);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[Client] Error in event loop: " << e.what() << std::endl;
            disconnect();
            break;
        }
    }

    std::cout << "[Client] Event loop stopped" << std::endl;
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
    ConnectRequest req(player_name);
    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::CONNECT_REQUEST, payload);
}

bool Client::sendMove(uint8_t column)
{
    if (session_token_.empty())
    {
        std::cerr << "[Client] Not authenticated (no session token)" << std::endl;
        return false;
    }

    MakeMoveRequest req(session_token_, column);
    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::MAKE_MOVE, payload);
}

bool Client::sendCreateGame(const GameConfig &config, const std::string &name)
{
    if (session_token_.empty())
    {
        std::cerr << "[Client] Not authenticated (no session token)" << std::endl;
        return false;
    }

    CreateGameRequest req;
    req.session_token = session_token_;
    req.game_name = name;
    req.config = config;

    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::CREATE_GAME, payload);
}

bool Client::sendJoinGame(uint32_t game_id)
{
    if (session_token_.empty())
    {
        std::cerr << "[Client] Not authenticated (no session token)" << std::endl;
        return false;
    }

    JoinGameRequest req(session_token_, game_id);
    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::JOIN_GAME, payload);
}

bool Client::sendListGames()
{
    if (session_token_.empty())
    {
        std::cerr << "[Client] Not authenticated (no session token)" << std::endl;
        return false;
    }

    ListGamesRequest req(session_token_);
    std::string payload = MessageSerializer::serialize(req);
    return sendMessage(MessageType::LIST_GAMES, payload);
}

bool Client::sendDisconnect(const std::string &reason)
{
    DisconnectMessage msg(reason);
    std::string payload = MessageSerializer::serialize(msg);
    return sendMessage(MessageType::DISCONNECT, payload);
}

// ============================================================================
// Internal Methods
// ============================================================================

bool Client::sendMessage(MessageType type, const std::string &payload)
{
    if (!connected_)
    {
        std::cerr << "[Client] Cannot send: not connected" << std::endl;
        return false;
    }

    std::string message = MessageSerializer::wrapMessage(type, payload);
    return writeMessage(message);
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
        std::cerr << "[Client] Invalid message length: " << length << std::endl;
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
        std::cerr << "[Client] Message too large: " << message.size() << std::endl;
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

        // Route to appropriate handler
        switch (type)
        {
        case MessageType::CONNECT_RESPONSE:
            handleConnectResponse(payload);
            break;

        case MessageType::CREATE_GAME_RESPONSE:
            handleCreateGameResponse(payload);
            break;

        case MessageType::JOIN_GAME_RESPONSE:
            handleJoinGameResponse(payload);
            break;

        case MessageType::GAME_LIST_RESPONSE:
            handleGameListResponse(payload);
            break;

        case MessageType::GAME_STATE_UPDATE:
            handleGameStateUpdate(payload);
            break;

        case MessageType::GAME_OVER:
            handleGameOver(payload);
            break;

        case MessageType::MOVE_RESULT:
            handleMoveResult(payload);
            break;

        case MessageType::ERROR:
            handleError(payload);
            break;

        default:
            std::cerr << "[Client] Unknown message type: "
                      << static_cast<int>(type) << std::endl;
            break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Client] Error processing message: " << e.what() << std::endl;
    }
}

// ============================================================================
// Message Handlers
// ============================================================================

void Client::handleConnectResponse(const std::string &payload)
{
    ConnectResponse resp = MessageSerializer::deserializeConnectResponse(payload);

    if (resp.success)
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        session_token_ = resp.session_token;

        std::cout << "[Client] Connected ... " << std::endl;

        if (connected_callback_)
        {
            connected_callback_();
        }
    }
    else
    {
        std::cerr << "[Client] Connection failed: " << resp.message << std::endl;

        if (error_callback_)
        {
            error_callback_(0, resp.message);
        }
    }
}

void Client::handleCreateGameResponse(const std::string &payload)
{
    CreateGameResponse resp = MessageSerializer::deserializeCreateGameResponse(payload);

    if (resp.success)
    {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            player_id_ = resp.assigned_player_id;
            game_info_ = resp.game_info;
        }

        std::cout << "[Client] Game created with ID " << resp.game_info.game_id
                  << ", assigned player ID " << (int)resp.assigned_player_id << std::endl;
    }
    else
    {
        std::cerr << "[Client] Create game failed: " << resp.message << std::endl;

        if (error_callback_)
        {
            error_callback_(0, resp.message);
        }
    }
}

void Client::handleJoinGameResponse(const std::string &payload)
{
    JoinGameResponse resp = MessageSerializer::deserializeJoinGameResponse(payload);

    if (resp.success)
    {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            player_id_ = resp.assigned_player_id;
            game_info_ = resp.game_info;
        }
        std::cout << "[Client] Joined game ID " << resp.game_info.game_id
                  << ", assigned player ID " << (int)resp.assigned_player_id << std::endl;
    }
    else
    {
        std::cerr << "[Client] Join game failed: " << resp.message << std::endl;

        if (error_callback_)
        {
            error_callback_(0, resp.message);
        }
    }
}

void Client::handleGameStateUpdate(const std::string &payload)
{
    GameStateUpdate update = MessageSerializer::deserializeGameStateUpdate(payload);

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        current_state_ = update;
    }

    if (game_state_callback_)
    {
        game_state_callback_(update);
    }
}

void Client::handleGameOver(const std::string &payload)
{
    GameOverMessage msg = MessageSerializer::deserializeGameOver(payload);

    std::cout << "[Client] Game Over: " << msg.message << std::endl;

    if (game_over_callback_)
    {
        game_over_callback_(msg);
    }
}

void Client::handleMoveResult(const std::string &payload)
{
    MoveResult result = MessageSerializer::deserializeMoveResult(payload);

    if (move_result_callback_)
    {
        move_result_callback_(result.success, result.message);
    }
}

void Client::handleError(const std::string &payload)
{
    ErrorMessage err = MessageSerializer::deserializeError(payload);

    std::cerr << "[Client] Server error " << err.error_code
              << ": " << err.error_message << std::endl;

    if (error_callback_)
    {
        error_callback_(err.error_code, err.error_message);
    }
}

void Client::handleGameListResponse(const std::string &payload)
{
    ListGamesResponse resp = MessageSerializer::deserializeGameListResponse(payload);

    if (game_list_callback_)
    {
        game_list_callback_(resp.games);
    }
}