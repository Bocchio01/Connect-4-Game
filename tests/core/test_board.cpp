// ============================================================================
// test_board.cpp - Tests for Board class
// ============================================================================

#include <iostream>
#include <cassert>
#include "core/board.hpp"

void test_board_initialization()
{
    std::cout << "Testing board initialization...";

    Board board(6, 7, 2);
    assert(board.getRows() == 6);
    assert(board.getCols() == 7);
    assert(board.getNumPlayers() == 2);

    // Check all positions are empty
    for (uint8_t r = 0; r < 6; ++r)
    {
        for (uint8_t c = 0; c < 7; ++c)
        {
            assert(board.get(r, c) == 0);
        }
    }

    std::cout << "passed" << std::endl;
}

void test_board_set_get()
{
    std::cout << "Testing board set/get...";

    Board board(6, 7, 2);
    board.set(5, 3, 1); // Bottom row, middle column, player 1
    assert(board.get(5, 3) == 1);

    board.set(4, 3, 2); // Stack player 2 on top
    assert(board.get(4, 3) == 2);
    assert(board.get(5, 3) == 1); // Player 1 still there

    std::cout << "passed" << std::endl;
}

void test_column_availability()
{
    std::cout << "Testing column availability...";

    Board board(6, 7, 2);

    // All columns should be available initially
    for (uint8_t c = 0; c < 7; ++c)
    {
        assert(board.isColumnAvailable(c));
    }

    // Fill column 3
    for (int r = 5; r >= 0; --r)
    {
        assert(board.isColumnAvailable(3));
        board.set(r, 3, 1);
    }

    // Column 3 should now be full
    assert(!board.isColumnAvailable(3));

    // Other columns should still be available
    assert(board.isColumnAvailable(2));
    assert(board.isColumnAvailable(4));

    std::cout << "passed" << std::endl;
}

void test_get_next_row()
{
    std::cout << "Testing getNextRow...";

    Board board(6, 7, 2);

    // Empty column should return bottom row
    auto row = board.getNextRow(3);
    assert(row.has_value());
    assert(row.value() == 5); // Bottom row (0-indexed from top)

    // Add a piece
    board.set(5, 3, 1);
    row = board.getNextRow(3);
    assert(row.has_value());
    assert(row.value() == 4); // Next row up

    // Fill the column
    for (uint8_t r = 0; r < 6; ++r)
    {
        board.set(r, 3, 1);
    }

    // Full column should return nullopt
    row = board.getNextRow(3);
    assert(!row.has_value());

    std::cout << "passed" << std::endl;
}

void test_board_full()
{
    std::cout << "Testing board full detection...";

    Board board(4, 4, 2);
    assert(!board.isFull());

    // Fill the board
    for (uint8_t r = 0; r < 4; ++r)
    {
        for (uint8_t c = 0; c < 4; ++c)
        {
            board.set(r, c, 1);
        }
    }

    assert(board.isFull());

    std::cout << "passed" << std::endl;
}

void test_board_clear()
{
    std::cout << "Testing board clear...";

    Board board(6, 7, 2);

    // Add some pieces
    board.set(5, 3, 1);
    board.set(4, 3, 2);
    board.set(5, 4, 1);

    assert(board.get(5, 3) == 1);

    // Clear the board
    board.clear();

    // Check all positions are empty
    for (uint8_t r = 0; r < 6; ++r)
    {
        for (uint8_t c = 0; c < 7; ++c)
        {
            assert(board.get(r, c) == 0);
        }
    }

    std::cout << "passed" << std::endl;
}

void test_custom_sizes()
{
    std::cout << "Testing custom board sizes...";

    // Small board
    Board small(4, 5, 2);
    assert(small.getRows() == 4);
    assert(small.getCols() == 5);

    // Large board
    Board large(10, 12, 3);
    assert(large.getRows() == 10);
    assert(large.getCols() == 12);
    assert(large.getNumPlayers() == 3);

    std::cout << "passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running Board Tests ===" << std::endl;

    try
    {
        test_board_initialization();
        test_board_set_get();
        test_column_availability();
        test_get_next_row();
        test_board_full();
        test_board_clear();
        test_custom_sizes();

        std::cout << "\n=== All Board tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
