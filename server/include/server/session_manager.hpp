#pragma once

#include <string>
#include <map>
#include <memory>
#include <optional>
#include <cstdint>
#include <mutex>
#include <random>

/**
 * Information about a client session
 */
struct ClientSession
{
    uint32_t connection_id;
    uint32_t player_id;
    uint32_t game_id;
    std::string session_token;
    std::string player_name;

    ClientSession() : connection_id(0), player_id(0), game_id(0) {}
};

/**
 * Manages client sessions and authentication tokens
 */
class SessionManager
{
public:
    SessionManager();

    /**
     * Create a new session for a connection
     * @return session token
     */
    std::string createSession(uint32_t connection_id, const std::string &player_name);

    /**
     * Validate a session token
     * @return connection_id if valid, nullopt otherwise
     */
    std::optional<uint32_t> validateToken(const std::string &token);

    /**
     * Get session by connection ID
     */
    std::optional<ClientSession> getSessionByConnection(uint32_t connection_id);

    /**
     * Get session by token
     */
    std::optional<ClientSession> getSessionByToken(const std::string &token);

    /**
     * Update session with game and player info
     */
    void assignToGame(const std::string &token, uint32_t game_id, uint32_t player_id);

    /**
     * Remove a session
     */
    void removeSession(uint32_t connection_id);

    /**
     * Remove session by token
     */
    void removeSessionByToken(const std::string &token);

private:
    std::string generateToken();

    std::map<std::string, ClientSession> sessions_;       // token -> session
    std::map<uint32_t, std::string> connection_to_token_; // connection_id -> token
    std::mutex mutex_;
    std::mt19937_64 rng_;
};