#include <iostream>
#include <algorithm>
#include "server/game_server.hpp"

GameServer::GameServer(uint16_t port)
    : port_(port),
      running_(false),
      next_connection_id_(1),
      next_game_id_(1)
{
}

GameServer::~GameServer()
{
    stop();
}

void GameServer::start()
{
    std::cout << "Starting Connect 4 Server on port " << port_ << "..." << std::endl;

    // Initialize sockpp library
    sockpp::socket_initializer::initialize();

    // Create acceptor
    if (!acceptor_.open(port_))
    {
        std::cerr << "Error creating acceptor: " << acceptor_.last_error_str() << std::endl;
        return;
    }

    running_ = true;
    std::cout << "Server started successfully!" << std::endl;
    std::cout << "Waiting for connections..." << std::endl;

    // Create default game
    createGame(6, 7, 2, 4);

    // Accept connections (blocking)
    acceptConnections();
}

void GameServer::stop()
{
    if (!running_)
    {
        return;
    }

    std::cout << "\nStopping server..." << std::endl;
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
    std::cout << "Server stopped" << std::endl;
}

void GameServer::acceptConnections()
{
    while (running_)
    {
        sockpp::inet_address peer;
        sockpp::tcp_socket socket = acceptor_.accept(&peer);

        if (!socket)
        {
            if (running_)
            {
                std::cerr << "Error accepting connection: "
                          << acceptor_.last_error_str() << std::endl;
            }
            continue;
        }

        uint32_t conn_id = next_connection_id_++;
        auto conn = std::make_shared<Connection>(std::move(socket), conn_id);

        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            connections_[conn_id] = conn;
        }

        std::cout << "[Connection " << conn_id << "] New connection from "
                  << peer << std::endl;

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

    // Start reading (blocks until disconnection)
    conn->start();
}

void GameServer::onMessage(std::shared_ptr<Connection> conn, const std::string &data)
{
    try
    {
        // Unwrap message
        auto [type, payload] = MessageSerializer::unwrapMessage(data);

        std::cout << "[Connection " << conn->getId() << "] Received: "
                  << messageTypeToString(type) << std::endl;

        // Route to appropriate handler
        switch (type)
        {
        case MessageType::CONNECT_REQUEST:
            handleConnectRequest(conn, payload);
            break;
        case MessageType::MAKE_MOVE:
            handleMakeMove(conn, payload);
            break;
        case MessageType::CREATE_GAME:
            handleCreateGame(conn, payload);
            break;
        case MessageType::JOIN_GAME:
            handleJoinGame(conn, payload);
            break;
        case MessageType::LIST_GAMES:
            handleListGames(conn, payload);
            break;
        case MessageType::DISCONNECT:
            handleDisconnect(conn, payload);
            break;
        default:
            sendError(conn, Protocol::ErrorCode::INVALID_MESSAGE,
                      "Unknown message type");
            break;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[Connection " << conn->getId() << "] Error processing message: "
                  << e.what() << std::endl;
        sendError(conn, Protocol::ErrorCode::INVALID_MESSAGE, "Malformed message");
    }
}

void GameServer::handleConnectRequest(std::shared_ptr<Connection> conn,
                                      const std::string &payload)
{
    ConnectRequest req = MessageSerializer::deserializeConnectRequest(payload);

    std::cout << "[Connection " << conn->getId() << "] Player '"
              << req.player_name << "' connecting..." << std::endl;

    // Create session
    std::string token = session_manager_.createSession(conn->getId(), req.player_name);

    // Find or create a game to join
    uint32_t game_id = 0;
    uint8_t player_id = 0;

    {
        std::lock_guard<std::mutex> lock(games_mutex_);

        // Find first available game
        for (auto &[id, game] : games_)
        {
            if (!game->isFull() &&
                game->getEngine().getState().getStatus() == GameStatus::NOT_STARTED)
            {
                game_id = id;
                player_id = game->addPlayer(conn->getId());
                break;
            }
        }

        // No available game, create new one
        if (game_id == 0)
        {
            game_id = createGame(6, 7, 2, 4);
            player_id = games_[game_id]->addPlayer(conn->getId());
        }
    }

    // Update session with game info
    session_manager_.assignToGame(token, game_id, player_id);

    // Send response
    ConnectResponse resp;
    resp.success = true;
    resp.assigned_player_id = player_id;
    resp.game_id = game_id;
    resp.session_token = token;
    resp.message = "Connected successfully";

    std::string respPayload = MessageSerializer::serialize(resp);
    sendWrappedMessage(conn, MessageType::CONNECT_RESPONSE, respPayload);

    std::cout << "[Connection " << conn->getId() << "] Assigned to game "
              << game_id << " as player " << (int)player_id << std::endl;

    // Broadcast game state to all players in this game
    broadcastGameState(game_id);
}

void GameServer::handleJoinGame(std::shared_ptr<Connection> conn,
                                const std::string &payload)
{
    JoinGameRequest req = MessageSerializer::deserializeJoinGameRequest(payload);

    // Validate token
    auto conn_id_opt = session_manager_.validateToken(req.session_token);
    if (!conn_id_opt.has_value())
    {
        sendError(conn, Protocol::ErrorCode::INVALID_SESSION_TOKEN,
                  "Invalid session token");
        return;
    }

    // Get game
    std::shared_ptr<GameSession> game;
    {
        std::lock_guard<std::mutex> lock(games_mutex_);
        auto it = games_.find(req.game_id);
        if (it == games_.end())
        {
            sendError(conn, Protocol::ErrorCode::GAME_NOT_FOUND, "Game not found");
            return;
        }
        game = it->second;
    }

    // Check if game is full
    if (game->isFull())
    {
        sendError(conn, Protocol::ErrorCode::GAME_FULL, "Game is full");
        return;
    }

    // Check if game already started
    if (game->getEngine().getState().getStatus() != GameStatus::NOT_STARTED)
    {
        sendError(conn, Protocol::ErrorCode::GAME_ALREADY_STARTED,
                  "Game already started");
        return;
    }

    // Add player to game
    uint8_t player_id = game->addPlayer(conn->getId());
    session_manager_.assignToGame(req.session_token, req.game_id, player_id);

    std::cout << "[Game " << req.game_id << "] Player " << (int)player_id
              << " joined" << std::endl;

    // Send success response
    ConnectResponse resp;
    resp.success = true;
    resp.assigned_player_id = player_id;
    resp.game_id = req.game_id;
    resp.message = "Joined game successfully";

    std::string respPayload = MessageSerializer::serialize(resp);
    sendWrappedMessage(conn, MessageType::CONNECT_RESPONSE, respPayload);

    // Broadcast game state
    broadcastGameState(req.game_id);
}

void GameServer::handleListGames(std::shared_ptr<Connection> conn,
                                 const std::string &payload)
{
    ListGamesRequest req = MessageSerializer::deserializeListGamesRequest(payload);

    // Validate token
    if (!session_manager_.validateToken(req.session_token).has_value())
    {
        sendError(conn, Protocol::ErrorCode::INVALID_SESSION_TOKEN,
                  "Invalid session token");
        return;
    }

    GameListResponse resp;

    {
        std::lock_guard<std::mutex> lock(games_mutex_);

        for (const auto &[id, game] : games_)
        {
            GameInfo info;
            info.game_id = id;
            info.current_players = game->getPlayerCount();
            info.max_players = game->getMaxPlayers();

            // Convert GameStatus to ProtocolGameStatus
            switch (game->getEngine().getState().getStatus())
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

            const auto &board = game->getEngine().getState().getBoard();
            info.config.rows = board.getRows();
            info.config.cols = board.getCols();
            info.config.num_players = board.getNumPlayers();
            info.config.connect_length = game->getEngine().getRules().getConnectLength();

            resp.games.push_back(info);
        }
    }

    std::string respPayload = MessageSerializer::serialize(resp);
    sendWrappedMessage(conn, MessageType::GAME_LIST_RESPONSE, respPayload);
}

void GameServer::handleDisconnect(std::shared_ptr<Connection> conn,
                                  const std::string &payload)
{
    DisconnectMessage msg = MessageSerializer::deserializeDisconnect(payload);

    std::cout << "[Connection " << conn->getId() << "] Disconnect request: "
              << msg.reason << std::endl;

    onDisconnect(conn->getId());
    conn->close();
}

void GameServer::onDisconnect(uint32_t connection_id)
{
    std::cout << "[Connection " << connection_id << "] Disconnected" << std::endl;

    // Get session info
    auto session_opt = session_manager_.getSessionByConnection(connection_id);

    if (session_opt.has_value())
    {
        auto session = session_opt.value();

        // Remove from game
        if (session.game_id != 0)
        {
            std::lock_guard<std::mutex> lock(games_mutex_);
            auto it = games_.find(session.game_id);
            if (it != games_.end())
            {
                it->second->removePlayer(connection_id);

                std::cout << "[Game " << session.game_id << "] Player "
                          << (int)session.player_id << " left" << std::endl;

                // If game is empty, remove it
                if (it->second->getPlayerCount() == 0)
                {
                    std::cout << "[Game " << session.game_id << "] Removed (empty)"
                              << std::endl;
                    games_.erase(it);
                }
            }
        }

        // Remove session
        session_manager_.removeSession(connection_id);
    }

    // Remove connection
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(connection_id);
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

void GameServer::sendError(std::shared_ptr<Connection> conn, uint16_t error_code,
                           const std::string &message)
{
    ErrorMessage err(error_code, message);
    std::string payload = MessageSerializer::serialize(err);
    sendWrappedMessage(conn, MessageType::ERROR, payload);

    std::cout << "[Connection " << conn->getId() << "] Error sent: "
              << error_code << " - " << message << std::endl;
}

void GameServer::sendWrappedMessage(std::shared_ptr<Connection> conn,
                                    MessageType type, const std::string &payload)
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

uint32_t GameServer::createGame(uint8_t rows, uint8_t cols, uint8_t num_players,
                                uint8_t connect_length)
{
    uint32_t game_id = next_game_id_++;

    auto game = std::make_shared<GameSession>(game_id, rows, cols,
                                              num_players, connect_length);

    std::lock_guard<std::mutex> lock(games_mutex_);
    games_[game_id] = game;

    return game_id;
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
        auto player_id = game.getPlayerId(conn_id);
        if (player_id.has_value())
        {
            update.players.push_back(player_id.value());
        }
    }

    return update;
}

void GameServer::handleMakeMove(std::shared_ptr<Connection> conn,
                                const std::string &payload)
{
    MakeMoveRequest req = MessageSerializer::deserializeMakeMoveRequest(payload);

    // Validate token
    auto conn_id_opt = session_manager_.validateToken(req.session_token);
    if (!conn_id_opt.has_value() || conn_id_opt.value() != conn->getId())
    {
        sendError(conn, Protocol::ErrorCode::INVALID_SESSION_TOKEN,
                  "Invalid session token");
        return;
    }

    // Get session info
    auto session_opt = session_manager_.getSessionByConnection(conn->getId());
    if (!session_opt.has_value())
    {
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
            sendError(conn, Protocol::ErrorCode::GAME_NOT_FOUND, "Game not found");
            return;
        }
        game = it->second;
    }

    // Make move
    bool success = game->getEngine().makeMove(req.column, session.player_id);

    if (success)
    {
        std::cout << "[Game " << session.game_id << "] Player "
                  << (int)session.player_id << " played column "
                  << (int)req.column << std::endl;

        // Send move result
        MoveResult result(true, "Move successful");
        std::string resultPayload = MessageSerializer::serialize(result);
        sendWrappedMessage(conn, MessageType::MOVE_RESULT, resultPayload);

        // Broadcast updated game state
        broadcastGameState(session.game_id);

        // Check if game is over
        if (game->getEngine().isGameOver())
        {
            GameOverMessage gameOver;
            gameOver.game_id = session.game_id;

            if (game->getEngine().getState().getStatus() == GameStatus::FINISHED_WIN)
            {
                gameOver.final_status = ProtocolGameStatus::FINISHED_WIN;
                gameOver.winner = game->getEngine().getWinner();
                gameOver.message = "Player " +
                                   std::to_string(gameOver.winner.value()) + " wins!";

                std::cout << "[Game " << session.game_id << "] Player "
                          << (int)gameOver.winner.value() << " wins!" << std::endl;
            }
            else
            {
                gameOver.final_status = ProtocolGameStatus::FINISHED_DRAW;
                gameOver.message = "Game ended in a draw!";

                std::cout << "[Game " << session.game_id << "] Draw!" << std::endl;
            }

            std::string gameOverPayload = MessageSerializer::serialize(gameOver);

            // Send to all players in game
            for (uint32_t player_conn_id : game->getConnections())
            {
                std::lock_guard<std::mutex> lock(connections_mutex_);
                auto player_it = connections_.find(player_conn_id);
                if (player_it != connections_.end())
                {
                    sendWrappedMessage(player_it->second, MessageType::GAME_OVER,
                                       gameOverPayload);
                }
            }
        }
    }
    else
    {
        std::cout << "[Game " << session.game_id << "] Player "
                  << (int)session.player_id << " invalid move to column "
                  << (int)req.column << std::endl;

        // Determine specific error
        if (game->getEngine().getState().getCurrentPlayer() != session.player_id)
        {
            sendError(conn, Protocol::ErrorCode::NOT_YOUR_TURN, "Not your turn");
        }
        else if (game->getEngine().isGameOver())
        {
            sendError(conn, Protocol::ErrorCode::GAME_ALREADY_OVER, "Game is over");
        }
        else
        {
            sendError(conn, Protocol::ErrorCode::INVALID_MOVE,
                      "Invalid move (column full or out of range)");
        }
    }
}

void GameServer::handleCreateGame(std::shared_ptr<Connection> conn,
                                  const std::string &payload)
{
    CreateGameRequest req = MessageSerializer::deserializeCreateGameRequest(payload);

    // Validate token
    if (!session_manager_.validateToken(req.session_token).has_value())
    {
        sendError(conn, Protocol::ErrorCode::INVALID_SESSION_TOKEN,
                  "Invalid session token");
        return;
    }

    // Validate config
    if (req.config.rows < Protocol::MIN_BOARD_SIZE ||
        req.config.rows > Protocol::MAX_BOARD_SIZE ||
        req.config.cols < Protocol::MIN_BOARD_SIZE ||
        req.config.cols > Protocol::MAX_BOARD_SIZE ||
        req.config.num_players < 2 ||
        req.config.num_players > Protocol::MAX_PLAYERS)
    {
        sendError(conn, Protocol::ErrorCode::INVALID_MESSAGE,
                  "Invalid game configuration");
        return;
    }

    // Create game
    uint32_t game_id = createGame(req.config.rows, req.config.cols,
                                  req.config.num_players,
                                  req.config.connect_length);

    std::cout << "[Server] Created game " << game_id << " with config: "
              << (int)req.config.rows << "x" << (int)req.config.cols
              << ", " << (int)req.config.num_players << " players" << std::endl;

    // Send success response (reuse ConnectResponse structure)
    ConnectResponse resp;
    resp.success = true;
    resp.game_id = game_id;
    resp.message = "Game created successfully";
    std::string respPayload = MessageSerializer::serialize(resp);
    sendWrappedMessage(conn, MessageType::CONNECT_RESPONSE, respPayload);
}