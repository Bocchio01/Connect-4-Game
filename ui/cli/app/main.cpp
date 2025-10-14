#include "core/board.hpp"
#include "client/client.hpp"

void printBoard(const Board &board);
void printBoard(const std::vector<std::vector<uint8_t>> &grid);

int main()
{
    Client client;

    client.onGameStateUpdate([](auto &state)
                             { printBoard(state.board); });

    client.connect("localhost", 8080);
    client.sendConnectRequest("Player");

    std::thread([&]()
                { client.run(); })
        .detach();

    while (client.isConnected())
    {
        int col;
        std::cin >> col;
        client.sendMove(col);
    }
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

void printBoard(const std::vector<std::vector<uint8_t>> &grid)
{
    for (const auto &row : grid)
    {
        for (const auto &cell : row)
        {
            std::cout << (int)cell << " ";
        }
        std::cout << std::endl;
    }
}