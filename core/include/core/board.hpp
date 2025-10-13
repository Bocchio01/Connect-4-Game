#pragma once

#include <vector>
#include <cstdint>
#include <optional>

/**
 * Represents the game board state
 * Supports variable dimensions and number of players
 */
class Board
{
public:
    /**
     * Constructor
     * @param rows Number of rows (height)
     * @param cols Number of columns (width)
     * @param num_players Number of players (default 2)
     */
    Board(uint8_t rows = 6, uint8_t cols = 7, uint8_t num_players = 2);

    /**
     * Get the value at a specific position
     * @return 0 for empty, 1..N for player N
     */
    uint8_t get(uint8_t row, uint8_t col) const;

    /**
     * Set the value at a specific position
     * @param row Row index (0-based)
     * @param col Column index (0-based)
     * @param player_id Player ID (1..N)
     */
    void set(uint8_t row, uint8_t col, uint8_t player_id);

    /**
     * Check if a column has space for a new piece
     */
    bool isColumnAvailable(uint8_t col) const;

    /**
     * Get the next available row in a column (for dropping piece)
     * @return Row index or std::nullopt if column is full
     */
    std::optional<uint8_t> getNextRow(uint8_t col) const;

    /**
     * Check if the board is completely full
     */
    bool isFull() const;

    /**
     * Reset the board to empty state
     */
    void clear();

    // Getters
    uint8_t getRows() const { return rows_; }
    uint8_t getCols() const { return cols_; }
    uint8_t getNumPlayers() const { return num_players_; }

    /**
     * Get raw board data (for serialization/display)
     */
    const std::vector<std::vector<uint8_t>> &getData() const { return grid_; }

private:
    uint8_t rows_;
    uint8_t cols_;
    uint8_t num_players_;
    std::vector<std::vector<uint8_t>> grid_; // 0 = empty, 1..N = player
};