#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <map>
#include "core/game/engine.hpp"

/**
 * Represents a single game instance on the server
 */
class GameSession
{
public:
    GameSession(uint32_t id, uint8_t rows, uint8_t cols,
                uint8_t num_players, uint8_t connect_length);

    /**
     * Get game ID
     */
    uint32_t getId() const { return id_; }

    /**
     * Get game name
     */
    const std::string &getName() const { return name_; }

    /**
     * Set game name
     */
    void setName(const std::string &name) { name_ = name; }

    /**
     * Get game engine
     */
    GameEngine &getEngine() { return engine_; }
    const GameEngine &getEngine() const { return engine_; }

    /**
     * Add a player to this game
     * @return assigned player_id or 0 if game is full
     */
    uint8_t addPlayer(uint32_t connection_id);

    /**
     * Remove a player from this game
     */
    void removePlayer(uint32_t connection_id);

    /**
     * Get player ID for a connection
     */
    std::optional<uint8_t> getPlayerId(uint32_t connection_id) const;

    /**
     * Get all connection IDs in this game
     */
    const std::vector<uint32_t> &getConnections() const { return connections_; }

    /**
     * Check if game is full
     */
    bool isFull() const { return connections_.size() >= max_players_; }

    /**
     * Get current player count
     */
    size_t getPlayerCount() const { return connections_.size(); }

    /**
     * Get max players
     */
    uint8_t getMaxPlayers() const { return max_players_; }

private:
    uint32_t id_;
    std::string name_;
    GameEngine engine_;
    std::vector<uint32_t> connections_;                // connection_id for each player
    std::map<uint32_t, uint8_t> connection_to_player_; // connection_id -> player_id
    uint8_t max_players_;
    uint8_t next_player_id_;
};