#pragma once

#include <map>

// ANSI color codes
namespace Color
{
    inline constexpr int RESET = 0;
    inline constexpr int RED = 31;
    inline constexpr int GREEN = 32;
    inline constexpr int YELLOW = 33;
    inline constexpr int BLUE = 34;
    inline constexpr int MAGENTA = 35;
    inline constexpr int CYAN = 36;
    inline constexpr int WHITE = 37;
    inline constexpr int BRIGHT_RED = 91;
    inline constexpr int BRIGHT_GREEN = 92;
    inline constexpr int BRIGHT_YELLOW = 93;
    inline constexpr int BRIGHT_BLUE = 94;
}

// Symbols
enum class Symbol : char
{
    EMPTY = '.',
    PLAYER1 = '#',
    PLAYER2 = '@',
    PLAYER3 = '%',
    PLAYER4 = '&',
    PLAYER5 = '*',
    PLAYER6 = 'o',
    PLAYER7 = '+',
    PLAYER8 = '=',
    PLAYER9 = 'x',
    PLAYER10 = '~'
};

struct PlayerVisual
{
    Symbol symbol;
    int color;
};

extern const std::map<int, PlayerVisual> PLAYER_VISUALS;
