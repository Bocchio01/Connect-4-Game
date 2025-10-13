#include "core/game/engine.hpp"

GameEngine::GameEngine(uint8_t rows, uint8_t cols, uint8_t num_players, uint8_t connect_length)
    : state_(rows, cols, num_players),
      rules_(connect_length),
      state_change_callback_(nullptr)
{
}

void GameEngine::startGame()
{
    state_.setStatus(GameStatus::IN_PROGRESS);
    state_.setCurrentPlayer(1);
    notifyStateChange();
}

bool GameEngine::makeMove(uint8_t column)
{
    return makeMove(column, state_.getCurrentPlayer());
}

bool GameEngine::makeMove(uint8_t column, uint8_t player_id)
{
    // Validate game state
    if (state_.getStatus() != GameStatus::IN_PROGRESS)
    {
        return false;
    }

    // Validate it's this player's turn
    if (player_id != state_.getCurrentPlayer())
    {
        return false;
    }

    // Validate move is legal
    if (!rules_.isValidMove(state_.getBoard(), column))
    {
        return false;
    }

    // Execute move
    auto row = state_.getBoard().getNextRow(column);
    if (!row.has_value())
    {
        return false;
    }

    state_.getBoard().set(row.value(), column, player_id);
    state_.addMove(Move(column, player_id));

    // Check for game end
    checkGameEnd(player_id);

    // Advance to next player if game continues
    if (state_.getStatus() == GameStatus::IN_PROGRESS)
    {
        advancePlayer();
    }

    notifyStateChange();
    return true;
}

void GameEngine::reset()
{
    state_.reset();
    notifyStateChange();
}

bool GameEngine::isGameOver() const
{
    return state_.getStatus() == GameStatus::FINISHED_WIN ||
           state_.getStatus() == GameStatus::FINISHED_DRAW;
}

void GameEngine::advancePlayer()
{
    uint8_t next = state_.getCurrentPlayer() + 1;
    if (next > state_.getBoard().getNumPlayers())
    {
        next = 1;
    }
    state_.setCurrentPlayer(next);
}

void GameEngine::checkGameEnd(uint8_t last_player)
{
    // Check for win
    if (rules_.checkWin(state_.getBoard(), last_player))
    {
        state_.setStatus(GameStatus::FINISHED_WIN);
        state_.setWinner(last_player);
        return;
    }

    // Check for draw
    if (rules_.checkDraw(state_.getBoard()))
    {
        state_.setStatus(GameStatus::FINISHED_DRAW);
        return;
    }
}

void GameEngine::notifyStateChange()
{
    if (state_change_callback_)
    {
        state_change_callback_(state_);
    }
}