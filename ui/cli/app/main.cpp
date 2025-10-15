#include "core/board.hpp"
#include "client/client.hpp"

void printBoard(const Board &board);
void printBoard(const std::vector<std::vector<uint8_t>> &grid);
void gameStateUpdateCallback(const GameStateUpdate &state);

int main()
{
    Client client;

    client.onGameStateUpdate(gameStateUpdateCallback);

    client.connect("localhost", 8080);
    client.sendConnectRequest("Player");

    std::thread([&]()
                { client.run(); })
        .detach();

    while (client.isConnected())
    {
        // TODO: fix this busy wait
        if (!client.isMyTurn())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        int col;
        std::cout << "Enter column to drop piece (-1 to quit): ";
        std::cin >> col;
        if (col == -1)
        {
            break;
        }
        client.sendMove(col);
    }

    client.disconnect();
    return 0;
}

void printBoard(const Board &board)
{
    std::cout << std::endl;
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
    std::cout << std::endl;
    for (const auto &row : grid)
    {
        for (const auto &cell : row)
        {
            std::cout << (int)cell << " ";
        }
        std::cout << std::endl;
    }
}

void gameStateUpdateCallback(const GameStateUpdate &state)
{
    if (state.status == ProtocolGameStatus::IN_PROGRESS)
        printBoard(state.board);
}