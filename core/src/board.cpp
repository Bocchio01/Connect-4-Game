#include <stdexcept>
#include "core/board.hpp"

Board::Board(uint8_t rows, uint8_t cols, uint8_t num_players)
    : rows_(rows),
      cols_(cols),
      num_players_(num_players)
{

    if (rows < 4 || cols < 4)
    {
        throw std::invalid_argument("Board must be at least 4x4");
    }
    if (num_players < 2)
    {
        throw std::invalid_argument("Must have at least 2 players");
    }

    grid_.resize(rows_, std::vector<uint8_t>(cols_, 0));
}

uint8_t Board::get(uint8_t row, uint8_t col) const
{
    if (row >= rows_ || col >= cols_)
    {
        throw std::out_of_range("Board position out of range");
    }
    return grid_[row][col];
}

void Board::set(uint8_t row, uint8_t col, uint8_t player_id)
{
    if (row >= rows_ || col >= cols_)
    {
        throw std::out_of_range("Board position out of range");
    }
    if (player_id > num_players_)
    {
        throw std::invalid_argument("Invalid player ID");
    }
    grid_[row][col] = player_id;
}

bool Board::isColumnAvailable(uint8_t col) const
{
    if (col >= cols_)
    {
        return false;
    }
    return grid_[0][col] == 0; // Top row must be empty
}

std::optional<uint8_t> Board::getNextRow(uint8_t col) const
{
    if (col >= cols_ || !isColumnAvailable(col))
    {
        return std::nullopt;
    }

    // Find the lowest empty row
    for (int row = rows_ - 1; row >= 0; --row)
    {
        if (grid_[row][col] == 0)
        {
            return row;
        }
    }

    return std::nullopt;
}

bool Board::isFull() const
{
    for (uint8_t col = 0; col < cols_; ++col)
    {
        if (isColumnAvailable(col))
        {
            return false;
        }
    }
    return true;
}

void Board::clear()
{
    for (auto &row : grid_)
    {
        std::fill(row.begin(), row.end(), 0);
    }
}