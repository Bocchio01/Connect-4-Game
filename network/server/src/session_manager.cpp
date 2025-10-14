#include <sstream>
#include <iomanip>
#include "server/session_manager.hpp"

SessionManager::SessionManager()
{
    std::random_device rd;
    rng_.seed(rd());
}

std::string SessionManager::createSession(uint32_t connection_id,
                                          const std::string &player_name)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string token = generateToken();

    ClientSession session;
    session.connection_id = connection_id;
    session.session_token = token;
    session.player_name = player_name;
    session.player_id = 0; // Not assigned to game yet
    session.game_id = 0;

    sessions_[token] = session;
    connection_to_token_[connection_id] = token;

    return token;
}

std::optional<uint32_t> SessionManager::validateToken(const std::string &token)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(token);
    if (it != sessions_.end())
    {
        return it->second.connection_id;
    }

    return std::nullopt;
}

std::optional<ClientSession> SessionManager::getSessionByConnection(uint32_t connection_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = connection_to_token_.find(connection_id);
    if (it != connection_to_token_.end())
    {
        return sessions_[it->second];
    }

    return std::nullopt;
}

std::optional<ClientSession> SessionManager::getSessionByToken(const std::string &token)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(token);
    if (it != sessions_.end())
    {
        return it->second;
    }

    return std::nullopt;
}

void SessionManager::assignToGame(const std::string &token, uint32_t game_id,
                                  uint32_t player_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(token);
    if (it != sessions_.end())
    {
        it->second.game_id = game_id;
        it->second.player_id = player_id;
    }
}

void SessionManager::removeSession(uint32_t connection_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = connection_to_token_.find(connection_id);
    if (it != connection_to_token_.end())
    {
        sessions_.erase(it->second);
        connection_to_token_.erase(it);
    }
}

void SessionManager::removeSessionByToken(const std::string &token)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(token);
    if (it != sessions_.end())
    {
        connection_to_token_.erase(it->second.connection_id);
        sessions_.erase(it);
    }
}

std::string SessionManager::generateToken()
{
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t random1 = dist(rng_);
    uint64_t random2 = dist(rng_);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << random1
        << std::setw(16) << random2;

    return oss.str();
}