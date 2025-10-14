#include <algorithm>
#include "server/game_session.hpp"

GameSession::GameSession(uint32_t id, uint8_t rows, uint8_t cols,
                         uint8_t num_players, uint8_t connect_length)
    : id_(id),
      engine_(rows, cols, num_players, connect_length),
      max_players_(num_players),
      next_player_id_(1)
{
}

uint8_t GameSession::addPlayer(uint32_t connection_id)
{
    if (isFull())
    {
        return 0;
    }

    uint8_t player_id = next_player_id_++;
    connections_.push_back(connection_id);
    connection_to_player_[connection_id] = player_id;

    // Start game when all players joined
    if (connections_.size() == max_players_)
    {
        engine_.startGame();
    }

    return player_id;
}

void GameSession::removePlayer(uint32_t connection_id)
{
    auto it = std::find(connections_.begin(), connections_.end(), connection_id);
    if (it != connections_.end())
    {
        connections_.erase(it);
    }
    connection_to_player_.erase(connection_id);
}

std::optional<uint8_t> GameSession::getPlayerId(uint32_t connection_id) const
{
    auto it = connection_to_player_.find(connection_id);
    if (it != connection_to_player_.end())
    {
        return it->second;
    }
    return std::nullopt;
}