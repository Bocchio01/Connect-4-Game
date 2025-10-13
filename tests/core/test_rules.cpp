// ============================================================================
// test_rules.cpp - Tests for GameRules class
// ============================================================================

#include <iostream>
#include <cassert>
#include "core/board.hpp"
#include "core/game/rules.hpp"

void test_horizontal_win()
{
    std::cout << "Testing horizontal win detection...";

    Board board(6, 7, 2);
    GameRules rules(4);

    // Create horizontal line for player 1 (bottom row)
    board.set(5, 2, 1);
    board.set(5, 3, 1);
    board.set(5, 4, 1);
    assert(!rules.checkWin(board, 1)); // Only 3, not enough

    board.set(5, 5, 1);
    assert(rules.checkWin(board, 1));  // 4 in a row, win!
    assert(!rules.checkWin(board, 2)); // Player 2 hasn't won

    std::cout << "passed" << std::endl;
}

void test_vertical_win()
{
    std::cout << "Testing vertical win detection...";

    Board board(6, 7, 2);
    GameRules rules(4);

    // Create vertical line for player 2 (column 3)
    board.set(5, 3, 2);
    board.set(4, 3, 2);
    board.set(3, 3, 2);
    assert(!rules.checkWin(board, 2)); // Only 3

    board.set(2, 3, 2);
    assert(rules.checkWin(board, 2)); // 4 in a column, win!

    std::cout << "passed" << std::endl;
}

void test_diagonal_down_win()
{
    std::cout << "Testing diagonal downward win detection...";

    Board board(6, 7, 2);
    GameRules rules(4);

    // Create diagonal line going down-right
    board.set(2, 2, 1);
    board.set(3, 3, 1);
    board.set(4, 4, 1);
    board.set(5, 5, 1);

    assert(rules.checkWin(board, 1));

    std::cout << "passed" << std::endl;
}

void test_diagonal_up_win()
{
    std::cout << "Testing diagonal upward win detection...";

    Board board(6, 7, 2);
    GameRules rules(4);

    // Create diagonal line going up-right
    board.set(5, 2, 2);
    board.set(4, 3, 2);
    board.set(3, 4, 2);
    board.set(2, 5, 2);

    assert(rules.checkWin(board, 2));

    std::cout << "passed" << std::endl;
}

void test_no_false_positives()
{
    std::cout << "Testing no false win detection...";

    Board board(6, 7, 2);
    GameRules rules(4);

    // Interrupted horizontal line
    board.set(5, 1, 1);
    board.set(5, 2, 1);
    board.set(5, 3, 2); // Interrupted by player 2
    board.set(5, 4, 1);

    assert(!rules.checkWin(board, 1));
    assert(!rules.checkWin(board, 2));

    std::cout << "passed" << std::endl;
}

void test_draw_detection()
{
    std::cout << "Testing draw detection...";

    Board board(4, 4, 2);
    GameRules rules(4);

    assert(!rules.checkDraw(board)); // Empty board, not a draw

    // Fill board in a pattern with no winner
    // Pattern: 1,2,1,2 / 2,1,2,1 / 1,2,1,2 / 2,1,2,1
    for (uint8_t r = 0; r < 4; ++r)
    {
        for (uint8_t c = 0; c < 4; ++c)
        {
            uint8_t player = ((r + c) % 2) + 1;
            if (r == 3)
            {
                player = (player == 1) ? 2 : 1; // Swap last piece to avoid 4 in a row
            }
            board.set(r, c, player);
        }
    }

    assert(!rules.checkWin(board, 1));
    assert(!rules.checkWin(board, 2));
    assert(rules.checkDraw(board));

    std::cout << "passed" << std::endl;
}

void test_move_validation()
{
    std::cout << "Testing move validation...";

    Board board(6, 7, 2);
    GameRules rules(4);

    // Valid moves
    assert(rules.isValidMove(board, 0));
    assert(rules.isValidMove(board, 3));
    assert(rules.isValidMove(board, 6));

    // Invalid column numbers
    assert(!rules.isValidMove(board, 7));
    assert(!rules.isValidMove(board, 255));

    // Fill a column
    for (uint8_t r = 0; r < 6; ++r)
    {
        board.set(r, 3, 1);
    }

    // Column 3 should now be invalid
    assert(!rules.isValidMove(board, 3));

    std::cout << "passed" << std::endl;
}

void test_custom_connect_length()
{
    std::cout << "Testing custom connect length...";

    Board board(8, 8, 2);
    GameRules rules(5); // Need 5 in a row

    // 4 in a row shouldn't win
    board.set(5, 2, 1);
    board.set(5, 3, 1);
    board.set(5, 4, 1);
    board.set(5, 5, 1);
    assert(!rules.checkWin(board, 1));

    // 5 in a row should win
    board.set(5, 6, 1);
    assert(rules.checkWin(board, 1));

    std::cout << "passed" << std::endl;
}

void test_three_players()
{
    std::cout << "Testing with 3 players...";

    Board board(6, 7, 3);
    GameRules rules(4);

    // Player 3 gets 4 in a row
    board.set(5, 0, 3);
    board.set(5, 1, 3);
    board.set(5, 2, 3);
    board.set(5, 3, 3);

    assert(!rules.checkWin(board, 1));
    assert(!rules.checkWin(board, 2));
    assert(rules.checkWin(board, 3));

    std::cout << "passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running GameRules Tests ===" << std::endl;

    try
    {
        test_horizontal_win();
        test_vertical_win();
        test_diagonal_down_win();
        test_diagonal_up_win();
        test_no_false_positives();
        test_draw_detection();
        test_move_validation();
        test_custom_connect_length();
        test_three_players();

        std::cout << "\n=== All GameRules tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}