#pragma once

#include <vector>
#include <cstdint>
#include "core/board.hpp"
#include "core/move.hpp"

/**
 * Possible states of the game
 */
enum class GameStatus : uint8_t
{
    NOT_STARTED,
    IN_PROGRESS,
    FINISHED_WIN,
    FINISHED_DRAW
};

/**
 * Represents the complete state of a game
 */
class GameState
{
public:
    GameState(uint8_t rows, uint8_t cols, uint8_t num_players);

    const Board &getBoard() const { return board_; }
    Board &getBoard() { return board_; }

    uint8_t getCurrentPlayer() const { return current_player_; }
    void setCurrentPlayer(uint8_t player) { current_player_ = player; }

    GameStatus getStatus() const { return status_; }
    void setStatus(GameStatus status) { status_ = status; }

    std::optional<uint8_t> getWinner() const { return winner_; }
    void setWinner(uint8_t player) { winner_ = player; }

    const std::vector<Move> &getMoveHistory() const { return move_history_; }
    void addMove(const Move &move) { move_history_.push_back(move); }

    void reset();

private:
    Board board_;
    uint8_t current_player_;
    uint8_t num_players_;
    GameStatus status_;
    std::optional<uint8_t> winner_;
    std::vector<Move> move_history_;
};