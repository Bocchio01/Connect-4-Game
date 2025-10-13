#include <stdexcept>
#include "core/game/rules.hpp"

GameRules::GameRules(uint8_t connect_length) : connect_length_(connect_length)
{
    if (connect_length < 3)
    {
        throw std::invalid_argument("Connect length must be at least 3");
    }
}

bool GameRules::isValidMove(const Board &board, uint8_t column) const
{
    return column < board.getCols() && board.isColumnAvailable(column);
}

bool GameRules::checkWin(const Board &board, uint8_t player_id) const
{
    return checkHorizontal(board, player_id) ||
           checkVertical(board, player_id) ||
           checkDiagonalDown(board, player_id) ||
           checkDiagonalUp(board, player_id);
}

bool GameRules::checkDraw(const Board &board) const
{
    if (!board.isFull())
    {
        return false;
    }

    // Check if any player has won (shouldn't happen in normal play)
    for (uint8_t p = 1; p <= board.getNumPlayers(); ++p)
    {
        if (checkWin(board, p))
        {
            return false;
        }
    }

    return true;
}

bool GameRules::checkDirection(const Board &board, uint8_t row, uint8_t col,
                               int8_t row_delta, int8_t col_delta, uint8_t player_id) const
{
    uint8_t count = 0;
    int8_t r = row;
    int8_t c = col;

    while (r >= 0 && r < board.getRows() && c >= 0 && c < board.getCols())
    {
        if (board.get(r, c) == player_id)
        {
            count++;
            if (count >= connect_length_)
            {
                return true;
            }
        }
        else
        {
            count = 0;
        }

        r += row_delta;
        c += col_delta;
    }

    return false;
}

bool GameRules::checkHorizontal(const Board &board, uint8_t player_id) const
{
    for (uint8_t row = 0; row < board.getRows(); ++row)
    {
        if (checkDirection(board, row, 0, 0, 1, player_id))
        {
            return true;
        }
    }
    return false;
}

bool GameRules::checkVertical(const Board &board, uint8_t player_id) const
{
    for (uint8_t col = 0; col < board.getCols(); ++col)
    {
        if (checkDirection(board, 0, col, 1, 0, player_id))
        {
            return true;
        }
    }
    return false;
}

bool GameRules::checkDiagonalDown(const Board &board, uint8_t player_id) const
{
    // Check all starting positions on top row and left column
    for (uint8_t col = 0; col < board.getCols(); ++col)
    {
        if (checkDirection(board, 0, col, 1, 1, player_id))
        {
            return true;
        }
    }
    for (uint8_t row = 1; row < board.getRows(); ++row)
    {
        if (checkDirection(board, row, 0, 1, 1, player_id))
        {
            return true;
        }
    }
    return false;
}

bool GameRules::checkDiagonalUp(const Board &board, uint8_t player_id) const
{
    // Check all starting positions on bottom row and left column
    for (uint8_t col = 0; col < board.getCols(); ++col)
    {
        if (checkDirection(board, board.getRows() - 1, col, -1, 1, player_id))
        {
            return true;
        }
    }
    for (uint8_t row = 0; row < board.getRows() - 1; ++row)
    {
        if (checkDirection(board, row, 0, -1, 1, player_id))
        {
            return true;
        }
    }
    return false;
}
