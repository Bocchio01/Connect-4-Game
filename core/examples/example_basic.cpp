#include <iostream>
#include "core/game/engine.hpp"

void printBoard(const Board &board);

int main()
{
    // Create a 6x7 board with 2 players, need 4 in a row to win
    GameEngine engine(6, 7, 2, 4);

    // Register callback for state changes
    engine.setStateChangeCallback([](const GameState &state)
                                  {
        std::cout << "Current player: " << (int)state.getCurrentPlayer() << "\n";
        if (state.getStatus() == GameStatus::FINISHED_WIN)
        {
            std::cout << "Player " << (int)state.getWinner().value() << " wins!\n";
        } });

    // Start the game
    engine.startGame();

    while (!engine.isGameOver())
    {
        printBoard(engine.getState().getBoard());
        std::cout << "Player " << (int)engine.getState().getCurrentPlayer()
                  << " - Enter column: ";
        int col;
        std::cin >> col;

        if (engine.makeMove(col))
        {
            // Move successful
        }
        else
        {
            std::cout << "Invalid move!\n";
        }
    }

    return 0;
}

void printBoard(const Board &board)
{
    for (uint8_t r = 0; r < board.getRows(); ++r)
    {
        for (uint8_t c = 0; c < board.getCols(); ++c)
        {
            std::cout << (int)board.get(r, c) << " ";
        }
        std::cout << std::endl;
    }
}