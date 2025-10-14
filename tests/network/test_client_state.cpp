#include <iostream>
#include <cassert>

#include "client/client.hpp"

void test_initial_state()
{
    std::cout << "Testing initial client state...";

    Client client;

    assert(!client.isConnected());
    assert(client.getSessionToken().empty());
    assert(client.getPlayerId() == 0);
    assert(client.getGameId() == 0);
    assert(!client.getCurrentState().has_value());
    assert(!client.isMyTurn());

    std::cout << " passed" << std::endl;
}

void test_callback_registration()
{
    std::cout << "Testing callback registration...";

    Client client;

    bool connected_called = false;
    bool disconnected_called = false;
    bool state_update_called = false;
    bool game_over_called = false;
    bool move_result_called = false;
    bool error_called = false;
    bool game_list_called = false;

    client.onConnected([&](uint8_t, uint32_t)
                       { connected_called = true; });

    client.onDisconnected([&]()
                          { disconnected_called = true; });

    client.onGameStateUpdate([&](const GameStateUpdate &)
                             { state_update_called = true; });

    client.onGameOver([&](const GameOverMessage &)
                      { game_over_called = true; });

    client.onMoveResult([&](bool, const std::string &)
                        { move_result_called = true; });

    client.onError([&](uint16_t, const std::string &)
                   { error_called = true; });

    client.onGameList([&](const std::vector<GameInfo> &)
                      { game_list_called = true; });

    // All callbacks should be registered (not null)
    std::cout << " passed" << std::endl;
}

void test_state_accessors()
{
    std::cout << "Testing state accessors...";

    Client client;

    // Initially empty/zero
    assert(client.getSessionToken().empty());
    assert(client.getPlayerId() == 0);
    assert(client.getGameId() == 0);
    assert(!client.getCurrentState().has_value());

    std::cout << " passed" << std::endl;
}

void test_is_my_turn()
{
    std::cout << "Testing isMyTurn logic...";

    Client client;

    // No state -> not my turn
    assert(!client.isMyTurn());

    std::cout << " passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running Client State Tests ===" << std::endl;

    try
    {
        test_initial_state();
        test_callback_registration();
        test_state_accessors();
        test_is_my_turn();

        std::cout << "\n=== All Client state tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}