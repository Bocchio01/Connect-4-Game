// ============================================================================
// test_engine.cpp - Tests for GameEngine class
// ============================================================================

#include <iostream>
#include <cassert>
#include "core/game/engine.hpp"

void test_game_initialization()
{
    std::cout << "Testing game initialization...";

    GameEngine engine(6, 7, 2, 4);

    assert(engine.getState().getStatus() == GameStatus::NOT_STARTED);
    assert(engine.getState().getCurrentPlayer() == 1);
    assert(!engine.isGameOver());

    std::cout << "passed" << std::endl;
}

void test_game_start()
{
    std::cout << "Testing game start...";

    GameEngine engine(6, 7, 2, 4);
    engine.startGame();

    assert(engine.getState().getStatus() == GameStatus::IN_PROGRESS);
    assert(engine.getState().getCurrentPlayer() == 1);

    std::cout << "passed" << std::endl;
}

void test_make_move()
{
    std::cout << "Testing make move...";

    GameEngine engine(6, 7, 2, 4);
    engine.startGame();

    // Player 1 makes a move
    bool success = engine.makeMove(3);
    assert(success);
    assert(engine.getState().getCurrentPlayer() == 2); // Turn advances

    // Check the piece is on the board
    const Board &board = engine.getState().getBoard();
    assert(board.get(5, 3) == 1); // Bottom row, column 3

    std::cout << "passed" << std::endl;
}

void test_turn_enforcement()
{
    std::cout << "Testing turn enforcement...";

    GameEngine engine(6, 7, 2, 4);
    engine.startGame();

    // Player 1's turn
    bool success = engine.makeMove(3, 1);
    assert(success);

    // Try to move again as player 1 (not their turn)
    success = engine.makeMove(4, 1);
    assert(!success); // Should fail

    // Player 2's turn should work
    success = engine.makeMove(4, 2);
    assert(success);

    std::cout << "passed" << std::endl;
}

void test_invalid_moves()
{
    std::cout << "Testing invalid move rejection...";

    GameEngine engine(6, 7, 2, 4);
    engine.startGame();

    // Invalid column
    bool success = engine.makeMove(10);
    assert(!success);

    // Fill a column
    for (int i = 0; i < 6; ++i)
    {
        engine.makeMove(3); // Alternates between players
    }

    // Column 3 should now be full
    success = engine.makeMove(3);
    assert(!success);

    std::cout << "passed" << std::endl;
}

void test_win_detection()
{
    std::cout << "Testing win detection...";

    GameEngine engine(6, 7, 2, 4);
    engine.startGame();

    // Create a winning scenario for player 1
    // P1 plays columns: 0, 1, 2, 3 (horizontal win on bottom row)
    // P2 plays columns: 4, 5, 6 (to give P1 turns)

    engine.makeMove(0, 1); // P1
    engine.makeMove(4, 2); // P2
    engine.makeMove(1, 1); // P1
    engine.makeMove(5, 2); // P2
    engine.makeMove(2, 1); // P1
    engine.makeMove(6, 2); // P2
    engine.makeMove(3, 1); // P1 - winning move!

    assert(engine.isGameOver());
    assert(engine.getState().getStatus() == GameStatus::FINISHED_WIN);
    assert(engine.getState().getWinner().value() == 1);

    std::cout << "passed" << std::endl;
}

void test_draw_detection()
{
    std::cout << "Testing draw detection...";

    GameEngine engine(4, 4, 2, 4);
    engine.startGame();

    // Fill board in a pattern with no winner
    // Board: 1,2,1,2
    //        1,2,1,2
    //        2,1,2,1
    //        2,1,2,1
    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            engine.makeMove(col); // Alternates between players
        }
    }
    for (int row = 2; row < 4; ++row)
    {
        for (int col = 3; col >= 0; --col)
        {
            engine.makeMove(col); // Alternates between players
        }
    }

    assert(engine.isGameOver());
    assert(engine.getState().getStatus() == GameStatus::FINISHED_DRAW);
    assert(!engine.getState().getWinner().has_value());

    std::cout << "passed" << std::endl;
}

void test_move_after_game_over()
{
    std::cout << "Testing moves rejected after game over...";

    GameEngine engine(6, 7, 2, 4);
    engine.startGame();

    // Create a quick win
    engine.makeMove(0, 1);
    engine.makeMove(6, 2);
    engine.makeMove(1, 1);
    engine.makeMove(6, 2);
    engine.makeMove(2, 1);
    engine.makeMove(6, 2);
    engine.makeMove(3, 1); // P1 wins

    assert(engine.isGameOver());

    // Try to make another move
    bool success = engine.makeMove(4, 2);
    assert(!success); // Should be rejected

    std::cout << "passed" << std::endl;
}

void test_reset()
{
    std::cout << "Testing game reset...";

    GameEngine engine(6, 7, 2, 4);
    engine.startGame();

    // Make some moves
    engine.makeMove(3);
    engine.makeMove(4);
    engine.makeMove(3);

    // Reset
    engine.reset();

    assert(engine.getState().getStatus() == GameStatus::NOT_STARTED);
    assert(engine.getState().getCurrentPlayer() == 1);
    assert(engine.getState().getMoveHistory().empty());

    // Board should be clear
    const Board &board = engine.getState().getBoard();
    for (uint8_t r = 0; r < board.getRows(); ++r)
    {
        for (uint8_t c = 0; c < board.getCols(); ++c)
        {
            assert(board.get(r, c) == 0);
        }
    }

    std::cout << "passed" << std::endl;
}

void test_move_history()
{
    std::cout << "Testing move history tracking...";

    GameEngine engine(6, 7, 2, 4);
    engine.startGame();

    assert(engine.getState().getMoveHistory().empty());

    engine.makeMove(3, 1);
    engine.makeMove(4, 2);
    engine.makeMove(5, 1);

    const auto &history = engine.getState().getMoveHistory();
    assert(history.size() == 3);
    assert(history[0].column == 3 && history[0].player_id == 1);
    assert(history[1].column == 4 && history[1].player_id == 2);
    assert(history[2].column == 5 && history[2].player_id == 1);

    std::cout << "passed" << std::endl;
}

void test_state_callback()
{
    std::cout << "Testing state change callback...";

    GameEngine engine(6, 7, 2, 4);

    int callback_count = 0;
    uint8_t last_player = 0;

    engine.setStateChangeCallback([&](const GameState &state)
                                  {
        callback_count++;
        last_player = state.getCurrentPlayer(); });

    engine.startGame(); // Callback 1
    assert(callback_count == 1);

    engine.makeMove(3); // Callback 2
    assert(callback_count == 2);
    assert(last_player == 2); // Turn advanced to player 2

    std::cout << "passed" << std::endl;
}

void test_three_player_game()
{
    std::cout << "Testing three player game...";

    GameEngine engine(6, 7, 3, 4);
    engine.startGame();

    assert(engine.getState().getCurrentPlayer() == 1);

    engine.makeMove(3, 1);
    assert(engine.getState().getCurrentPlayer() == 2);

    engine.makeMove(4, 2);
    assert(engine.getState().getCurrentPlayer() == 3);

    engine.makeMove(5, 3);
    assert(engine.getState().getCurrentPlayer() == 1); // Wraps around

    std::cout << "passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running GameEngine Tests ===" << std::endl;

    try
    {
        test_game_initialization();
        test_game_start();
        test_make_move();
        test_turn_enforcement();
        test_invalid_moves();
        test_win_detection();
        test_draw_detection();
        test_move_after_game_over();
        test_reset();
        test_move_history();
        test_state_callback();
        test_three_player_game();

        std::cout << "\n=== All GameEngine tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}