#include <random>
#include "core/game/state.hpp"

GameState::GameState(uint8_t rows, uint8_t cols, uint8_t num_players)
    : board_(rows, cols, num_players),
      current_player_(1),
      num_players_(num_players),
      status_(GameStatus::NOT_STARTED),
      winner_(std::nullopt)
{
}

void GameState::reset()
{
    static std::random_device rd;  // seed generator (hardware entropy)
    static std::mt19937 gen(rd()); // Mersenne Twister RNG
    std::uniform_int_distribution<> dist(1, num_players_);

    board_.clear();
    current_player_ = dist(gen);
    status_ = GameStatus::NOT_STARTED;
    winner_ = std::nullopt;
    move_history_.clear();
}
