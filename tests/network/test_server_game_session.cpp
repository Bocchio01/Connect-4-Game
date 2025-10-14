#include <iostream>
#include <cassert>

#include "server/game_session.hpp"

void test_game_session_creation()
{
    std::cout << "Testing game session creation...";

    GameSession game(1, 6, 7, 2, 4);

    assert(game.getId() == 1);
    assert(game.getMaxPlayers() == 2);
    assert(game.getPlayerCount() == 0);
    assert(!game.isFull());

    const auto &engine = game.getEngine();
    assert(engine.getState().getBoard().getRows() == 6);
    assert(engine.getState().getBoard().getCols() == 7);

    std::cout << " passed" << std::endl;
}

void test_add_player()
{
    std::cout << "Testing add player...";

    GameSession game(1, 6, 7, 2, 4);

    // Add first player
    uint8_t player1 = game.addPlayer(100);
    assert(player1 == 1);
    assert(game.getPlayerCount() == 1);
    assert(!game.isFull());

    // Add second player
    uint8_t player2 = game.addPlayer(200);
    assert(player2 == 2);
    assert(game.getPlayerCount() == 2);
    assert(game.isFull());

    // Game should start when full
    assert(game.getEngine().getState().getStatus() == GameStatus::IN_PROGRESS);

    // Try to add third player (should fail)
    uint8_t player3 = game.addPlayer(300);
    assert(player3 == 0); // Failed
    assert(game.getPlayerCount() == 2);

    std::cout << " passed" << std::endl;
}

void test_get_player_id()
{
    std::cout << "Testing get player ID...";

    GameSession game(1, 6, 7, 2, 4);

    game.addPlayer(100);
    game.addPlayer(200);

    auto player1 = game.getPlayerId(100);
    assert(player1.has_value());
    assert(player1.value() == 1);

    auto player2 = game.getPlayerId(200);
    assert(player2.has_value());
    assert(player2.value() == 2);

    auto no_player = game.getPlayerId(999);
    assert(!no_player.has_value());

    std::cout << " passed" << std::endl;
}

void test_remove_player()
{
    std::cout << "Testing remove player...";

    GameSession game(1, 6, 7, 2, 4);

    game.addPlayer(100);
    game.addPlayer(200);

    assert(game.getPlayerCount() == 2);
    assert(game.isFull());

    // Remove player
    game.removePlayer(100);

    assert(game.getPlayerCount() == 1);
    assert(!game.isFull());
    assert(!game.getPlayerId(100).has_value());
    assert(game.getPlayerId(200).has_value());

    std::cout << " passed" << std::endl;
}

void test_get_connections()
{
    std::cout << "Testing get connections...";

    GameSession game(1, 6, 7, 3, 4);

    game.addPlayer(10);
    game.addPlayer(20);
    game.addPlayer(30);

    const auto &connections = game.getConnections();

    assert(connections.size() == 3);
    assert(connections[0] == 10);
    assert(connections[1] == 20);
    assert(connections[2] == 30);

    std::cout << " passed" << std::endl;
}

void test_custom_game_config()
{
    std::cout << "Testing custom game configuration...";

    // Large board, 3 players, need 5 in a row
    GameSession game(1, 10, 12, 3, 5);

    assert(game.getMaxPlayers() == 3);

    const auto &engine = game.getEngine();
    assert(engine.getState().getBoard().getRows() == 10);
    assert(engine.getState().getBoard().getCols() == 12);
    assert(engine.getRules().getConnectLength() == 5);

    // Add all players
    game.addPlayer(1);
    game.addPlayer(2);
    assert(!game.isFull());

    game.addPlayer(3);
    assert(game.isFull());
    assert(game.getEngine().getState().getStatus() == GameStatus::IN_PROGRESS);

    std::cout << " passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running GameSession Tests ===" << std::endl;

    try
    {
        test_game_session_creation();
        test_add_player();
        test_get_player_id();
        test_remove_player();
        test_get_connections();
        test_custom_game_config();

        std::cout << "\n=== All GameSession tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
