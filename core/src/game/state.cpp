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
    board_.clear();
    current_player_ = 1;
    status_ = GameStatus::NOT_STARTED;
    winner_ = std::nullopt;
    move_history_.clear();
}
