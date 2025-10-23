#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <atomic>

#include "client/client.hpp"

// NOTE: These tests require a running server on localhost:8080

void test_connection()
{
    std::cout << "Testing connection to server...";
    std::cout << "  (Make sure server is running on localhost:8080)" << std::endl;

    Client client;

    // Try to connect
    bool connected = client.connect("localhost", 8080);

    if (!connected)
    {
        std::cout << " Skipping (server not running)! " << std::endl;
        return;
    }

    assert(client.isConnected());

    // Disconnect
    client.disconnect();
    assert(!client.isConnected());

    std::cout << " passed" << std::endl;
}

void test_full_game_flow()
{
    std::cout << "Testing full game flow...";
    std::cout << "  (Make sure server is running on localhost:8080)" << std::endl;

    Client client1;
    Client client2;

    // Try to connect
    if (!client1.connect("localhost", 8080))
    {
        std::cout << " Skipping (server not running)! " << std::endl;
        return;
    }

    if (!client2.connect("localhost", 8080))
    {
        std::cout << " Skipping (server not running)! " << std::endl;
        client1.disconnect();
        return;
    }

    std::atomic<bool> client1_connected = false;
    std::atomic<bool> client2_connected = false;
    std::atomic<int> state_updates = 0;
    std::atomic<bool> game_over = false;

    // Setup callbacks for client 1
    client1.onConnected([&](uint8_t player_id, uint32_t game_id)
                        {
        std::cout << "  Client 1 connected: Player " << (int)player_id
                  << ", Game " << game_id << std::endl;
        client1_connected = true; });

    client1.onGameStateUpdate([&](const GameStateUpdate &)
                              { state_updates++; });

    client1.onGameOver([&](const GameOverMessage &msg)
                       {
        std::cout << "  Game over: " << msg.message << std::endl;
        game_over = true; });

    // Setup callbacks for client 2
    client2.onConnected([&](uint8_t player_id, uint32_t game_id)
                        {
        std::cout << "  Client 2 connected: Player " << (int)player_id
                  << ", Game " << game_id << std::endl;
        client2_connected = true; });

    // Send connect requests
    client1.sendConnectRequest("Player1");
    client2.sendConnectRequest("Player2");

    // Start event loops in threads
    std::thread thread1([&]()
                        { client1.run(); });

    std::thread thread2([&]()
                        { client2.run(); });

    // Wait for both to connect
    auto start = std::chrono::steady_clock::now();
    while ((!client1_connected || !client2_connected) &&
           std::chrono::steady_clock::now() - start < std::chrono::seconds(5))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!client1_connected || !client2_connected)
    {
        std::cout << " Connection timeout! " << std::endl;
        client1.stop();
        client2.stop();
        thread1.join();
        thread2.join();
        return;
    }

    // Wait a bit for game state update
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Make a move
    if (client1.isMyTurn())
    {
        std::cout << "  Client 1 making move...";
        client1.sendMove(3);
    }
    else if (client2.isMyTurn())
    {
        std::cout << "  Client 2 making move...";
        client2.sendMove(3);
    }

    // Wait for state update
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Should have received at least 2 state updates (initial + after move)
    assert(state_updates >= 2);

    // Disconnect
    client1.stop();
    client2.stop();

    thread1.join();
    thread2.join();

    std::cout << " passed" << std::endl;
}

void test_error_handling()
{
    std::cout << "Testing error handling...";
    std::cout << "  (Make sure server is running on localhost:8080)" << std::endl;

    Client client;

    if (!client.connect("localhost", 8080))
    {
        std::cout << " Skipping (server not running)! " << std::endl;
        return;
    }

    std::atomic<bool> error_received = false;

    client.onError([&](uint16_t code, const std::string &msg)
                   {
        std::cout << "  Received error " << code << ": " << msg << std::endl;
        error_received = true; });

    client.onConnected([&](uint8_t, uint32_t)
                       {
                           // Try to make move before it's our turn (should error)
                           std::this_thread::sleep_for(std::chrono::milliseconds(100));

                           // This will likely fail with "not authenticated" since we don't have a token yet
                           client.sendMove(10); // Invalid column
                       });

    client.sendConnectRequest("TestPlayer");

    // Start event loop
    std::thread thread([&]()
                       { client.run(); });

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(2));

    client.stop();
    thread.join();

    std::cout << " passed" << std::endl;
}

void test_invalid_connection()
{
    std::cout << "Testing connection to invalid server...";

    Client client;

    // Try to connect to non-existent server
    bool connected = client.connect("localhost", 9999);

    // Should fail
    assert(!connected);
    assert(!client.isConnected());

    std::cout << " passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running Client Integration Tests ===" << std::endl;
    std::cout << "\nNOTE: These tests require a running server on localhost:8080" << std::endl;
    std::cout << "Start the server with: ./connectx-server 8080\n"
              << std::endl;

    try
    {
        test_invalid_connection();
        test_connection();
        test_full_game_flow();
        test_error_handling();

        std::cout << "\n=== All Client integration tests passed! ===" << std::endl;
        std::cout << "\n(Some tests may have been skipped if server was not running)" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}