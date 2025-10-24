#include <algorithm>
#include <iostream>
#include <spdlog/spdlog.h>

#include "server/game_server.hpp"

GameServer::GameServer(uint16_t port) : port_(port), running_(false)
{
}

GameServer::~GameServer()
{
    stop();
}

void GameServer::start()
{
    spdlog::info("Starting Connect X Server...");

    // Initialize sockpp library
    sockpp::socket_initializer::initialize();

    // Create acceptor
    if (!acceptor_.open(port_))
    {
        spdlog::error("Error creating acceptor: {}", acceptor_.last_error_str());
        return;
    }

    // Start listening
    running_ = true;
    sockpp::inet_address addr = acceptor_.address();
    uint16_t port = addr.port();
    spdlog::info("Server started successfully on port {}", port);

    // Accept connections
    acceptConnections();
}

void GameServer::stop()
{
    if (!running_)
    {
        return;
    }

    spdlog::info("Stopping Connect X Server...");
    running_ = false;

    // Close all connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto &[id, conn] : connections_)
        {
            conn->close();
        }
        connections_.clear();
    }

    acceptor_.close();
    spdlog::info("Server stopped.");
}

void GameServer::acceptConnections()
{
    while (running_)
    {
        sockpp::inet_address peer;
        sockpp::tcp_socket socket = acceptor_.accept(&peer);

        spdlog::debug("Incoming connection from {}", peer.to_string());

        if (!socket)
        {
            if (running_)
                spdlog::error("Error accepting connection: {}", acceptor_.last_error_str());
            continue;
        }

        // Get connection ID
        uint32_t conn_id;
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            if (!free_connection_ids_.empty())
            {
                conn_id = free_connection_ids_.front();
                free_connection_ids_.pop();
            }
            else
            {
                conn_id = connections_.empty() ? 1 : (connections_.rbegin()->first + 1);
            }
        }

        auto conn = std::make_shared<Connection>(std::move(socket), conn_id);
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_[conn_id] = conn;
        }

        spdlog::debug("Accepted connection ID({}) from {}", conn_id, peer.to_string());

        // Start handling in new thread
        std::thread([this, conn]()
                    { handleConnection(conn); })
            .detach();
    }
}

void GameServer::handleConnection(std::shared_ptr<Connection> conn)
{
    // Set callbacks
    conn->setMessageCallback([this, conn](const std::string &data)
                             { onMessage(conn, data); });
    conn->setDisconnectCallback([this, conn]()
                                { onDisconnect(conn->getId()); });

    // Start connection
    conn->start();
}

void GameServer::onMessage(std::shared_ptr<Connection> conn, const std::string &data)
{
    try
    {
        // Unwrap message
        auto [type, payload] = MessageSerializer::unwrapMessage(data);

        spdlog::debug("Received message of type {} from connection ID({})", messageTypeToString(type), conn->getId());

        // Route to appropriate handler
        switch (type)
        {
        case MessageType::REQ_CONNECT:
            handleConnectRequest(conn, payload);
            break;
        case MessageType::REQ_CREATE_GAME:
            handleCreateGame(conn, payload);
            break;
        case MessageType::REQ_LIST_GAMES:
            handleListGames(conn, payload);
            break;
        case MessageType::REQ_JOIN_GAME:
            handleJoinGame(conn, payload);
            break;
        case MessageType::MAKE_MOVE:
            handleMakeMove(conn, payload);
            break;
        case MessageType::DISCONNECT:
            handleDisconnect(conn, payload);
            break;
        default:
            sendError(conn, Protocol::ErrorCode::INVALID_MESSAGE, "Unknown message type");
            break;
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("Error processing message from connection ID({}): {}", conn->getId(), e.what());
        sendError(conn, Protocol::ErrorCode::INVALID_MESSAGE, "Malformed message");
    }
}

void GameServer::onDisconnect(uint32_t connection_id)
{
    spdlog::debug("Connection ID({}) disconnected", connection_id);

    // Retrieve session
    auto session_opt = session_manager_.getSessionByConnection(connection_id);
    if (!session_opt.has_value())
        return;

    auto session = session_opt.value();
    std::shared_ptr<GameSession> game = nullptr;

    // Handle associated game
    if (session.game_id != 0)
    {
        {
            std::lock_guard<std::mutex> lock(games_mutex_);
            auto it = games_.find(session.game_id);
            if (it != games_.end())
            {
                game = it->second; // keep shared_ptr alive after unlock
                spdlog::debug("Player ID({}) removed from game ID({})",
                              (int)session.player_id, session.game_id);

                game->removePlayer(connection_id);

                if (game->getPlayerCount() == 0)
                {
                    spdlog::debug("Game ID({}) removed (empty)", it->first);
                    free_game_ids_.push(it->first);
                    games_.erase(it);
                    game.reset();
                }
            }
        }

        if (game)
        {
            spdlog::debug("Broadcasting updated game state for game ID({})", session.game_id);
            skipAbandonedPlayers(game);
            broadcastGameState(session.game_id);
        }
    }

    // Remove session
    session_manager_.removeSession(connection_id);

    // Remove connection
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(connection_id);
        free_connection_ids_.push(connection_id);
    }
}

void GameServer::handleConnectRequest(std::shared_ptr<Connection> conn, const std::string &payload)
{
    ConnectRequest req = MessageSerializer::deserializeConnectRequest(payload);

    spdlog::debug("Connection ID({}) requests connection as player '{}'", conn->getId(), req.player_name);

    // Create session
    std::string token = session_manager_.createSession(conn->getId(), req.player_name);

    // Send response
    ConnectResponse resp;
    resp.success = true;
    resp.session_token = token;
    resp.message = "Connected successfully";

    std::string respPayload = MessageSerializer::serialize(resp);
    sendWrappedMessage(conn, MessageType::RES_CONNECT, respPayload);
}

void GameServer::handleCreateGame(std::shared_ptr<Connection> conn, const std::string &payload)
{
    CreateGameRequest req = MessageSerializer::deserializeCreateGameRequest(payload);

    spdlog::debug("Connection ID({}) requested new game ('{}')<{}, {}, {}, {}>", conn->getId(), req.game_name,
                  (int)req.config.rows, (int)req.config.cols, (int)req.config.num_players,
                  (int)req.config.connect_length);

    // Validate token
    auto conn_id_opt = session_manager_.validateToken(req.session_token);
    if (!conn_id_opt.has_value())
    {
        spdlog::debug("Invalid session token from connection ID({})", conn->getId());
        sendError(conn, Protocol::ErrorCode::INVALID_SESSION_TOKEN, "Invalid session token");
        return;
    }

    // Check that game name is not empty
    if (!req.game_name.empty())
    {
        std::lock_guard<std::mutex> lock(games_mutex_);
        auto it = std::find_if(games_.begin(), games_.end(),
                               [&req](const auto &pair)
                               { return pair.second->getName() == req.game_name; });
        if (it != games_.end())
        {
            spdlog::debug("Game name '{}' already exists", req.game_name);
            sendError(conn, Protocol::ErrorCode::INVALID_MESSAGE, "Game name already exists");
            return;
        }
    }

    // Validate config
    if (req.config.rows < Protocol::MIN_BOARD_SIZE || req.config.rows > Protocol::MAX_BOARD_SIZE ||
        req.config.cols < Protocol::MIN_BOARD_SIZE || req.config.cols > Protocol::MAX_BOARD_SIZE ||
        req.config.num_players < 2 || req.config.num_players > Protocol::MAX_PLAYERS)
    {
        spdlog::debug("Invalid game configuration from connection ID({})", conn->getId());
        sendError(conn, Protocol::ErrorCode::INVALID_MESSAGE, "Invalid game configuration");
        return;
    }

    // Create game
    uint32_t game_id = createGame(req.config.rows, req.config.cols, req.config.num_players, req.config.connect_length, req.game_name);

    spdlog::debug("Game ID({}) created by connection ID({})", game_id, conn->getId());

    // Send response
    CreateGameResponse resp;
    resp.success = true;
    resp.game_info = getGameInfo(game_id, *games_[game_id]);
    resp.message = "Game created successfully";

    std::string respPayload = MessageSerializer::serialize(resp);
    sendWrappedMessage(conn, MessageType::RES_CREATE_GAME, respPayload);

    // Broadcast game state
    broadcastGameState(game_id);
}

void GameServer::handleListGames(std::shared_ptr<Connection> conn, const std::string &payload)
{
    ListGamesRequest req = MessageSerializer::deserializeListGamesRequest(payload);

    spdlog::debug("Connection ID({}) requested game list", conn->getId());

    ListGamesResponse resp;

    {
        std::lock_guard<std::mutex> lock(games_mutex_);

        for (const auto &[id, game] : games_)
        {
            GameInfo info;
            info = getGameInfo(id, *game);
            resp.games.push_back(info);
        }
    }

    spdlog::debug("Sending game list with {} games to connection ID({})", resp.games.size(), conn->getId());

    std::string respPayload = MessageSerializer::serialize(resp);
    sendWrappedMessage(conn, MessageType::RES_LIST_GAMES, respPayload);
}

void GameServer::handleJoinGame(std::shared_ptr<Connection> conn, const std::string &payload)
{
    JoinGameRequest req = MessageSerializer::deserializeJoinGameRequest(payload);

    spdlog::debug("Connection ID({}) requests to join game ID({})", conn->getId(), req.game_id);

    // Validate token
    auto conn_id_opt = session_manager_.validateToken(req.session_token);
    if (!conn_id_opt.has_value())
    {
        spdlog::debug("Invalid session token from connection ID({})", conn->getId());
        sendError(conn, Protocol::ErrorCode::INVALID_SESSION_TOKEN, "Invalid session token");
        return;
    }

    std::shared_ptr<GameSession> game;
    if (req.game_id == 0)
    {
        // Auto-join: find first available game
        {
            std::lock_guard<std::mutex> lock(games_mutex_);
            for (const auto &[id, g] : games_)
            {
                // Need to enforce passkey for private games. For now, just skip named games.
                if (!g->isFull() && g->getEngine().getState().getStatus() == GameStatus::NOT_STARTED && g->getName().empty())
                {
                    game = g;
                    req.game_id = id;
                    break;
                }
            }
        }
        if (!game)
        {
            // Create a standard game if none available
            req.game_id = createGame(6, 7, 2, 4, ""); // Default config
            game = games_[req.game_id];
        }
    }
    else
    {
        // Get game
        std::lock_guard<std::mutex> lock(games_mutex_);
        auto it = games_.find(req.game_id);
        if (it == games_.end())
        {
            spdlog::debug("Game ID({}) not found for connection ID({})", req.game_id, conn->getId());
            sendError(conn, Protocol::ErrorCode::GAME_NOT_FOUND, "Game not found");
            return;
        }
        game = it->second;
    }

    // Check if game is full
    if (game->isFull())
    {
        spdlog::debug("Game ID({}) is full for connection ID({})", req.game_id, conn->getId());
        sendError(conn, Protocol::ErrorCode::GAME_FULL, "Game is full");
        return;
    }

    // Check if game already started
    if (game->getEngine().getState().getStatus() != GameStatus::NOT_STARTED)
    {
        spdlog::debug("Game ID({}) already started for connection ID({})", req.game_id, conn->getId());
        sendError(conn, Protocol::ErrorCode::GAME_ALREADY_STARTED, "Game already started");
        return;
    }

    // Add player to game
    uint8_t player_id = game->addPlayer(conn->getId());
    session_manager_.assignToGame(req.session_token, req.game_id, player_id);

    spdlog::debug("Connection ID({}) joined game ID({}) as player ID({})", conn->getId(), req.game_id, (int)player_id);

    // Send response
    JoinGameResponse resp;
    resp.success = true;
    resp.assigned_player_id = player_id;
    resp.game_info = getGameInfo(req.game_id, *game);
    resp.message = "Joined game successfully";

    std::string respPayload = MessageSerializer::serialize(resp);
    sendWrappedMessage(conn, MessageType::RES_JOIN_GAME, respPayload);

    // Broadcast game state
    broadcastGameState(req.game_id);
}

void GameServer::handleMakeMove(std::shared_ptr<Connection> conn, const std::string &payload)
{
    MakeMoveRequest req = MessageSerializer::deserializeMakeMoveRequest(payload);

    spdlog::debug("Connection ID({}) makes move to column {}", conn->getId(), (int)req.column);

    // Validate token
    auto conn_id_opt = session_manager_.validateToken(req.session_token);
    if (!conn_id_opt.has_value() || conn_id_opt.value() != conn->getId())
    {
        spdlog::debug("Invalid session token from connection ID({})", conn->getId());
        sendError(conn, Protocol::ErrorCode::INVALID_SESSION_TOKEN, "Invalid session token");
        return;
    }

    // Get session info
    auto session_opt = session_manager_.getSessionByConnection(conn->getId());
    if (!session_opt.has_value())
    {
        spdlog::debug("Session not found for connection ID({})", conn->getId());
        sendError(conn, Protocol::ErrorCode::SESSION_EXPIRED, "Session not found");
        return;
    }

    auto session = session_opt.value();

    // Get game
    std::shared_ptr<GameSession> game;
    {
        std::lock_guard<std::mutex> lock(games_mutex_);
        auto it = games_.find(session.game_id);
        if (it == games_.end())
        {
            spdlog::debug("Game ID({}) not found for connection ID({})", session.game_id, conn->getId());
            sendError(conn, Protocol::ErrorCode::GAME_NOT_FOUND, "Game not found");
            return;
        }
        game = it->second;
    }

    // Make move
    bool success = game->getEngine().makeMove(req.column, session.player_id);

    if (success)
    {
        spdlog::debug("Player ID({}) in game ID({}) made move to column {}", (int)session.player_id, session.game_id, (int)req.column);

        // Send move result
        MoveResult result(true, "Move successful");
        std::string resultPayload = MessageSerializer::serialize(result);
        sendWrappedMessage(conn, MessageType::MOVE_RESULT, resultPayload);

        // Check if we have to skip some players (in case of abandoned players)
        skipAbandonedPlayers(game);

        // Broadcast updated game state
        broadcastGameState(session.game_id);
    }
    else
    {
        spdlog::debug("Invalid move by player ID({}) in game ID({}) to column {}", (int)session.player_id, session.game_id, (int)req.column);

        // Determine specific error
        if (game->getEngine().getState().getCurrentPlayer() != session.player_id)
        {
            spdlog::debug("It's not player ID({})'s turn in game ID({})", (int)session.player_id, session.game_id);
            sendError(conn, Protocol::ErrorCode::NOT_YOUR_TURN, "Not your turn");
        }
        else if (game->getEngine().isGameOver())
        {
            spdlog::debug("Game ID({}) is already over for player ID({})", session.game_id, (int)session.player_id);
            sendError(conn, Protocol::ErrorCode::GAME_ALREADY_OVER, "Game is over");
        }
        else
        {
            spdlog::debug("Move to column {} is invalid for player ID({}) in game ID({})", (int)req.column, (int)session.player_id, session.game_id);
            MoveResult result(false, "Invalid move (column full or out of range)");
            std::string resultPayload = MessageSerializer::serialize(result);
            sendWrappedMessage(conn, MessageType::MOVE_RESULT, resultPayload);
        }
    }
}

void GameServer::handleDisconnect(std::shared_ptr<Connection> conn, const std::string &payload)
{
    DisconnectMessage msg = MessageSerializer::deserializeDisconnect(payload);

    spdlog::debug("Connection ID({}) requests disconnect: {}", conn->getId(), msg.reason);

    onDisconnect(conn->getId());
    conn->close();
}

// ============================================================================
// Helper Functions
// ============================================================================

void GameServer::sendError(std::shared_ptr<Connection> conn, uint16_t error_code, const std::string &message)
{
    ErrorMessage err(error_code, message);
    std::string payload = MessageSerializer::serialize(err);
    sendWrappedMessage(conn, MessageType::ERROR, payload);
}

void GameServer::sendWrappedMessage(std::shared_ptr<Connection> conn, MessageType type, const std::string &payload)
{
    std::string message = MessageSerializer::wrapMessage(type, payload);
    conn->send(message);
}

void GameServer::broadcastGameState(uint32_t game_id)
{
    std::shared_ptr<GameSession> game;

    {
        std::lock_guard<std::mutex> lock(games_mutex_);
        auto it = games_.find(game_id);
        if (it == games_.end())
        {
            return;
        }
        game = it->second;
    }

    // Create state update
    GameStateUpdate update = createGameStateUpdate(*game);
    std::string payload = MessageSerializer::serialize(update);

    // Send to all players in game
    for (uint32_t conn_id : game->getConnections())
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto it = connections_.find(conn_id);
        if (it != connections_.end())
        {
            sendWrappedMessage(it->second, MessageType::GAME_STATE_UPDATE, payload);
        }
    }
}

uint32_t GameServer::createGame(uint8_t rows, uint8_t cols, uint8_t num_players, uint8_t connect_length,
                                std::string game_name)
{
    std::lock_guard<std::mutex> lock(games_mutex_);

    uint32_t game_id;
    if (!free_game_ids_.empty())
    {
        game_id = free_game_ids_.front();
        free_game_ids_.pop();
    }
    else
    {
        game_id = games_.empty() ? 1 : (games_.rbegin()->first + 1);
    }

    auto game = std::make_shared<GameSession>(game_id, rows, cols, num_players, connect_length, game_name);

    games_[game_id] = game;

    return game_id;
}

GameInfo GameServer::getGameInfo(uint32_t game_id, const GameSession &game)
{
    GameInfo info;

    info.game_id = game_id;
    info.game_name = game.getName();
    info.current_players = game.getPlayerCount();

    switch (game.getEngine().getState().getStatus())
    {
    case GameStatus::NOT_STARTED:
        info.status = ProtocolGameStatus::NOT_STARTED;
        break;
    case GameStatus::IN_PROGRESS:
        info.status = ProtocolGameStatus::IN_PROGRESS;
        break;
    case GameStatus::FINISHED_WIN:
        info.status = ProtocolGameStatus::FINISHED_WIN;
        break;
    case GameStatus::FINISHED_DRAW:
        info.status = ProtocolGameStatus::FINISHED_DRAW;
        break;
    }

    const auto &board = game.getEngine().getState().getBoard();
    info.config.rows = board.getRows();
    info.config.cols = board.getCols();
    info.config.num_players = board.getNumPlayers();
    info.config.connect_length = game.getEngine().getRules().getConnectLength();

    return info;
}

GameStateUpdate GameServer::createGameStateUpdate(const GameSession &game)
{
    GameStateUpdate update;

    const auto &state = game.getEngine().getState();
    const auto &board = state.getBoard();

    update.game_id = game.getId();
    update.rows = board.getRows();
    update.cols = board.getCols();
    update.current_player = state.getCurrentPlayer();
    update.board = board.getData();

    // Convert GameStatus to ProtocolGameStatus
    switch (state.getStatus())
    {
    case GameStatus::NOT_STARTED:
        update.status = ProtocolGameStatus::NOT_STARTED;
        break;
    case GameStatus::IN_PROGRESS:
        update.status = ProtocolGameStatus::IN_PROGRESS;
        break;
    case GameStatus::FINISHED_WIN:
        update.status = ProtocolGameStatus::FINISHED_WIN;
        update.winner = state.getWinner();
        break;
    case GameStatus::FINISHED_DRAW:
        update.status = ProtocolGameStatus::FINISHED_DRAW;
        break;
    }

    // Add all player IDs
    for (uint32_t conn_id : game.getConnections())
    {
        // Get player name from session manager
        auto session_opt = session_manager_.getSessionByConnection(conn_id);
        if (session_opt.has_value())
        {
            auto session = session_opt.value();
            update.players.push_back(session.player_id);
            update.player_names.push_back(session.player_name);
        }
    }

    return update;
}

void GameServer::skipAbandonedPlayers(const std::shared_ptr<GameSession> &game)
{
    std::vector<uint8_t> player_ids;
    auto connections = game->getConnections();
    for (uint32_t conn_id : connections)
    {
        auto session_opt = session_manager_.getSessionByConnection(conn_id);
        if (session_opt.has_value())
        {
            player_ids.push_back(session_opt->player_id);
        }
    }
    while (std::find(player_ids.begin(), player_ids.end(), game->getEngine().getState().getCurrentPlayer()) == player_ids.end())
    {
        spdlog::debug("Skipping abandoned player ID({}) in game ID({})", (int)game->getEngine().getState().getCurrentPlayer(), game->getId());
        game->getEngine().skipPlayerTurn();
    }
}