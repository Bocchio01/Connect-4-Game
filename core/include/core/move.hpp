#pragma once

#include <cstdint>

/**
 * Represents a single move in the game
 */
struct Move
{
    uint8_t column;
    uint8_t player_id;

    Move() : column(0), player_id(0) {}
    Move(uint8_t col, uint8_t player) : column(col), player_id(player) {}
};