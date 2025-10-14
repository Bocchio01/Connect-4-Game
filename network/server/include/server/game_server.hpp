#pragma once

#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdint>
#include <sockpp/tcp_acceptor.h>

#include "protocol/protocol.hpp"
#include "server/connection.hpp"
#include "server/session_manager.hpp"
#include "server/game_session.hpp"

/**
 * Main game server
 * Handles client connections, game sessions, and message routing
 */
class GameServer
{
public:
    explicit GameServer(uint16_t port = Protocol::DEFAULT_PORT);
    ~GameServer();

    /**
     * Start the server (blocking)
     */
    void start();

    /**
     * Stop the server
     */
    void stop();

    /**
     * Check if server is running
     */
    bool isRunning() const { return running_; }

private:
    /**
     * Accept incoming connections (runs in main thread)
     */
    void acceptConnections();

    /**
     * Handle a client connection (runs in separate thread)
     */
    void handleConnection(std::shared_ptr<Connection> conn);

    /**
     * Process a message from a client
     */
    void onMessage(std::shared_ptr<Connection> conn, const std::string &data);

    /**
     * Handle client disconnection
     */
    void onDisconnect(uint32_t connection_id);

    // Message handlers
    void handleConnectRequest(std::shared_ptr<Connection> conn, const std::string &payload);
    void handleJoinGame(std::shared_ptr<Connection> conn, const std::string &payload);
    void handleListGames(std::shared_ptr<Connection> conn, const std::string &payload);
    void handleDisconnect(std::shared_ptr<Connection> conn, const std::string &payload);
    void handleMakeMove(std::shared_ptr<Connection> conn, const std::string &payload);
    void handleCreateGame(std::shared_ptr<Connection> conn, const std::string &payload);

    // Helper functions
    void sendError(std::shared_ptr<Connection> conn, uint16_t error_code,
                   const std::string &message);
    void sendWrappedMessage(std::shared_ptr<Connection> conn,
                            MessageType type, const std::string &payload);
    void broadcastGameState(uint32_t game_id);
    uint32_t createGame(uint8_t rows, uint8_t cols, uint8_t num_players,
                        uint8_t connect_length);
    GameStateUpdate createGameStateUpdate(const GameSession &game);

    uint16_t port_;

    sockpp::tcp_acceptor acceptor_;
    SessionManager session_manager_;

    std::map<uint32_t, std::shared_ptr<Connection>> connections_;
    std::map<uint32_t, std::shared_ptr<GameSession>> games_;

    std::atomic<bool> running_;
    std::atomic<uint32_t> next_connection_id_;
    std::atomic<uint32_t> next_game_id_;

    std::mutex connections_mutex_;
    std::mutex games_mutex_;
};