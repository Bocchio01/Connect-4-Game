#include "cli/visual.hpp"

const std::map<int, PlayerVisual> PLAYER_VISUALS = {
    {0, {Symbol::EMPTY, Color::WHITE}},
    {1, {Symbol::PLAYER1, Color::RED}},
    {2, {Symbol::PLAYER2, Color::BLUE}},
    {3, {Symbol::PLAYER3, Color::YELLOW}},
    {4, {Symbol::PLAYER4, Color::MAGENTA}},
    {5, {Symbol::PLAYER5, Color::GREEN}},
    {6, {Symbol::PLAYER6, Color::CYAN}},
    {7, {Symbol::PLAYER7, Color::BRIGHT_RED}},
    {8, {Symbol::PLAYER8, Color::BRIGHT_BLUE}},
    {9, {Symbol::PLAYER9, Color::BRIGHT_GREEN}},
    {10, {Symbol::PLAYER10, Color::BRIGHT_YELLOW}}};
