#pragma once

#include <cstdint>
#include <optional>
#include "core/board.hpp"

/**
 * Encapsulates game rules and win condition checking
 */
class GameRules
{
public:
    /**
     * Constructor
     * @param connect_length Number of consecutive pieces needed to win (default 4)
     */
    explicit GameRules(uint8_t connect_length = 4);

    /**
     * Check if a specific player has won
     * @param board Current board state
     * @param player_id Player to check
     * @return true if player has won
     */
    bool checkWin(const Board &board, uint8_t player_id) const;

    /**
     * Check if the game is a draw (board full, no winner)
     */
    bool checkDraw(const Board &board) const;

    /**
     * Validate if a move is legal
     */
    bool isValidMove(const Board &board, uint8_t column) const;

    uint8_t getConnectLength() const { return connect_length_; }

private:
    uint8_t connect_length_;

    bool checkDirection(const Board &board, uint8_t row, uint8_t col,
                        int8_t row_delta, int8_t col_delta, uint8_t player_id) const;

    bool checkHorizontal(const Board &board, uint8_t player_id) const;
    bool checkVertical(const Board &board, uint8_t player_id) const;
    bool checkDiagonalDown(const Board &board, uint8_t player_id) const;
    bool checkDiagonalUp(const Board &board, uint8_t player_id) const;
};