#include <iostream>
#include <cassert>
#include <thread>
#include <vector>

#include "server/session_manager.hpp"

void test_create_session()
{
    std::cout << "Testing session creation...";

    SessionManager manager;

    std::string token = manager.createSession(1, "Alice");

    assert(!token.empty());
    assert(token.length() == 32); // 16 bytes hex = 32 chars

    std::cout << " Token: " << token << " " << std::endl;
    std::cout << " passed" << std::endl;
}

void test_validate_token()
{
    std::cout << "Testing token validation...";

    SessionManager manager;

    std::string token = manager.createSession(1, "Bob");

    // Valid token
    auto conn_id = manager.validateToken(token);
    assert(conn_id.has_value());
    assert(conn_id.value() == 1);

    // Invalid token
    auto invalid = manager.validateToken("invalid_token_12345");
    assert(!invalid.has_value());

    std::cout << " passed" << std::endl;
}

void test_get_session()
{
    std::cout << "Testing get session...";

    SessionManager manager;

    std::string token = manager.createSession(42, "Charlie");

    // Get by connection ID
    auto session1 = manager.getSessionByConnection(42);
    assert(session1.has_value());
    assert(session1->connection_id == 42);
    assert(session1->player_name == "Charlie");
    assert(session1->session_token == token);

    // Get by token
    auto session2 = manager.getSessionByToken(token);
    assert(session2.has_value());
    assert(session2->connection_id == 42);
    assert(session2->player_name == "Charlie");

    // Non-existent session
    auto no_session = manager.getSessionByConnection(999);
    assert(!no_session.has_value());

    std::cout << " passed" << std::endl;
}

void test_assign_to_game()
{
    std::cout << "Testing assign to game...";

    SessionManager manager;

    std::string token = manager.createSession(1, "Dave");

    // Initially not assigned
    auto session = manager.getSessionByToken(token);
    assert(session->game_id == 0);
    assert(session->player_id == 0);

    // Assign to game
    manager.assignToGame(token, 123, 2);

    // Check assignment
    session = manager.getSessionByToken(token);
    assert(session->game_id == 123);
    assert(session->player_id == 2);

    std::cout << " passed" << std::endl;
}

void test_remove_session()
{
    std::cout << "Testing session removal...";

    SessionManager manager;

    std::string token = manager.createSession(5, "Eve");

    // Session exists
    assert(manager.validateToken(token).has_value());

    // Remove by connection ID
    manager.removeSession(5);

    // Session no longer exists
    assert(!manager.validateToken(token).has_value());
    assert(!manager.getSessionByConnection(5).has_value());

    // Test remove by token
    std::string token2 = manager.createSession(6, "Frank");
    assert(manager.validateToken(token2).has_value());

    manager.removeSessionByToken(token2);
    assert(!manager.validateToken(token2).has_value());

    std::cout << " passed" << std::endl;
}

void test_unique_tokens()
{
    std::cout << "Testing token uniqueness...";

    SessionManager manager;

    std::vector<std::string> tokens;

    // Create 100 sessions
    for (int i = 0; i < 100; ++i)
    {
        std::string token = manager.createSession(i, "User" + std::to_string(i));
        tokens.push_back(token);
    }

    // Check all tokens are unique
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        for (size_t j = i + 1; j < tokens.size(); ++j)
        {
            assert(tokens[i] != tokens[j]);
        }
    }

    std::cout << " Created 100 unique tokens " << std::endl;
    std::cout << " passed" << std::endl;
}

void test_thread_safety()
{
    std::cout << "Testing thread safety...";

    SessionManager manager;

    const int NUM_THREADS = 10;
    const int SESSIONS_PER_THREAD = 10;

    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> all_tokens(NUM_THREADS);

    // Create sessions from multiple threads
    for (int t = 0; t < NUM_THREADS; ++t)
    {
        threads.emplace_back([&manager, &all_tokens, t]()
                             {
            for (int i = 0; i < SESSIONS_PER_THREAD; ++i) {
                uint32_t conn_id = t * SESSIONS_PER_THREAD + i;
                std::string token = manager.createSession(conn_id, "User");
                all_tokens[t].push_back(token);
            } });
    }

    // Wait for all threads
    for (auto &thread : threads)
    {
        thread.join();
    }

    // Validate all sessions
    int valid_count = 0;
    for (const auto &tokens : all_tokens)
    {
        for (const auto &token : tokens)
        {
            if (manager.validateToken(token).has_value())
            {
                valid_count++;
            }
        }
    }

    assert(valid_count == NUM_THREADS * SESSIONS_PER_THREAD);

    std::cout << "  Created " << valid_count << " sessions from "
              << NUM_THREADS << " threads" << std::endl;
    std::cout << " passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running SessionManager Tests ===" << std::endl;

    try
    {
        test_create_session();
        test_validate_token();
        test_get_session();
        test_assign_to_game();
        test_remove_session();
        test_unique_tokens();
        test_thread_safety();

        std::cout << "\n=== All SessionManager tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}