#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <optional>
#include <sockpp/tcp_connector.h>

#include "protocol/protocol.hpp"

/**
 * High-level client for Connect X game
 * Handles connection, message serialization, and state management
 */
class Client
{
public:
    // Callback types
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;
    using GameStateUpdateCallback = std::function<void(const GameStateUpdate &)>;
    using GameOverCallback = std::function<void(const GameOverMessage &)>;
    using MoveResultCallback = std::function<void(bool success, const std::string &message)>;
    using ErrorCallback = std::function<void(uint16_t error_code, const std::string &message)>;
    using GameListCallback = std::function<void(const std::vector<GameInfo> &games)>;

    Client();
    ~Client();

    // ========================================================================
    // Connection Management
    // ========================================================================

    /**
     * Connect to server
     * @param host Server hostname or IP
     * @param port Server port
     * @return true if connection successful
     */
    bool connect(const std::string &host, uint16_t port);

    /**
     * Disconnect from server
     */
    void disconnect();

    /**
     * Check if connected to server
     */
    bool isConnected() const { return connected_; }

    /**
     * Run the client event loop (blocking)
     * Processes incoming messages until disconnected
     */
    void run();

    /**
     * Stop the event loop
     */
    void stop();

    // ========================================================================
    // Game Actions
    // ========================================================================

    /**
     * Send connection request with player name
     * @param player_name Your display name
     */
    bool sendConnectRequest(const std::string &player_name);

    /**
     * Create a new game with custom configuration
     */
    bool sendCreateGame(const GameConfig &config, const std::string &name = "");

    /**
     * Request list of available games
     */
    bool sendListGames();

    /**
     * Join an existing game by ID
     */
    bool sendJoinGame(uint32_t game_id);

    /**
     * Make a move (place piece in column)
     * @param column Column index (0-based)
     */
    bool sendMove(uint8_t column);

    /**
     * Send disconnect message
     */
    bool sendDisconnect(const std::string &reason = "Client disconnecting");

    // ========================================================================
    // Callbacks (set these to handle events)
    // ========================================================================

    void onConnected(ConnectedCallback callback)
    {
        connected_callback_ = callback;
    }

    void onDisconnected(DisconnectedCallback callback)
    {
        disconnected_callback_ = callback;
    }

    void onGameStateUpdate(GameStateUpdateCallback callback)
    {
        game_state_callback_ = callback;
    }

    void onGameOver(GameOverCallback callback)
    {
        game_over_callback_ = callback;
    }

    void onMoveResult(MoveResultCallback callback)
    {
        move_result_callback_ = callback;
    }

    void onError(ErrorCallback callback)
    {
        error_callback_ = callback;
    }

    void onGameList(GameListCallback callback)
    {
        game_list_callback_ = callback;
    }

    // ========================================================================
    // State Accessors
    // ========================================================================

    /**
     * Get current session token
     */
    const std::string &getSessionToken() const { return session_token_; }

    /**
     * Get assigned player ID
     */
    uint8_t getPlayerId() const { return player_id_; }

    /**
     * Get current game info
     */
    const std::optional<GameInfo> &getGameInfo() const { return game_info_; }

    /**
     * Get current game ID
     */
    uint32_t getGameId() const { return game_info_.has_value() ? game_info_->game_id : 0; }

    /**
     * Get current game name
     */
    std::string getGameName() const { return game_info_.has_value() ? game_info_->game_name : std::string(); }

    /**
     * Get current game state (updated by server)
     */
    const std::optional<GameStateUpdate> &getCurrentState() const
    {
        return current_state_;
    }

    /**
     * Check if it's my turn
     */
    bool isMyTurn() const
    {
        if (!current_state_.has_value() || current_state_->status != ProtocolGameStatus::IN_PROGRESS)
            return false;
        return current_state_->current_player == player_id_;
    }

private:
    // Network
    sockpp::tcp_connector socket_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;

    // Session state
    std::string session_token_;
    uint8_t player_id_;
    std::optional<GameInfo> game_info_;
    std::optional<GameStateUpdate> current_state_;
    std::mutex state_mutex_;

    // Callbacks
    ConnectedCallback connected_callback_;
    DisconnectedCallback disconnected_callback_;
    GameStateUpdateCallback game_state_callback_;
    GameOverCallback game_over_callback_;
    MoveResultCallback move_result_callback_;
    ErrorCallback error_callback_;
    GameListCallback game_list_callback_;

    // Internal methods
    bool sendMessage(MessageType type, const std::string &payload);
    std::string readMessage();
    bool writeMessage(const std::string &message);
    void processMessage(const std::string &data);

    // Message handlers
    void handleConnectResponse(const std::string &payload);
    void handleCreateGameResponse(const std::string &payload);
    void handleJoinGameResponse(const std::string &payload);
    void handleGameStateUpdate(const std::string &payload);
    void handleGameOver(const std::string &payload);
    void handleMoveResult(const std::string &payload);
    void handleError(const std::string &payload);
    void handleGameListResponse(const std::string &payload);
};