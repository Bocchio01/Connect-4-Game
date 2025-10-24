#include <iostream>
#include <cassert>

#include "protocol/protocol.hpp"
#include "client/client.hpp"

void test_connect_request_format()
{
    std::cout << "Testing connect request format...";

    // Simulate what Client would send
    ConnectRequest req("TestPlayer");
    std::string payload = MessageSerializer::serialize(req);
    std::string message = MessageSerializer::wrapMessage(MessageType::REQ_CONNECT, payload);

    // Verify it can be unwrapped and deserialized
    auto [type, recv_payload] = MessageSerializer::unwrapMessage(message);
    assert(type == MessageType::REQ_CONNECT);

    ConnectRequest recv_req = MessageSerializer::deserializeConnectRequest(recv_payload);
    assert(recv_req.player_name == "TestPlayer");
    assert(recv_req.protocol_version == Protocol::VERSION);

    std::cout << " passed" << std::endl;
}

void test_move_request_format()
{
    std::cout << "Testing move request format...";

    std::string token = "test_token_123";
    uint8_t column = 3;

    MakeMoveRequest req(token, column);
    std::string payload = MessageSerializer::serialize(req);
    std::string message = MessageSerializer::wrapMessage(MessageType::MAKE_MOVE, payload);

    // Verify
    auto [type, recv_payload] = MessageSerializer::unwrapMessage(message);
    assert(type == MessageType::MAKE_MOVE);

    MakeMoveRequest recv_req = MessageSerializer::deserializeMakeMoveRequest(recv_payload);
    assert(recv_req.session_token == token);
    assert(recv_req.column == column);

    std::cout << " passed" << std::endl;
}

void test_create_game_format()
{
    std::cout << "Testing create game format...";

    CreateGameRequest req;
    req.session_token = "token123";
    req.config = GameConfig(8, 10, 3, 5);

    std::string payload = MessageSerializer::serialize(req);
    std::string message = MessageSerializer::wrapMessage(MessageType::REQ_CREATE_GAME, payload);

    // Verify
    auto [type, recv_payload] = MessageSerializer::unwrapMessage(message);
    assert(type == MessageType::REQ_CREATE_GAME);

    CreateGameRequest recv_req = MessageSerializer::deserializeCreateGameRequest(recv_payload);
    assert(recv_req.session_token == "token123");
    assert(recv_req.config.rows == 8);
    assert(recv_req.config.cols == 10);
    assert(recv_req.config.num_players == 3);
    assert(recv_req.config.connect_length == 5);

    std::cout << " passed" << std::endl;
}

void test_join_game_format()
{
    std::cout << "Testing join game format...";

    JoinGameRequest req("token456", 42);
    std::string payload = MessageSerializer::serialize(req);
    std::string message = MessageSerializer::wrapMessage(MessageType::REQ_JOIN_GAME, payload);

    // Verify
    auto [type, recv_payload] = MessageSerializer::unwrapMessage(message);
    assert(type == MessageType::REQ_JOIN_GAME);

    JoinGameRequest recv_req = MessageSerializer::deserializeJoinGameRequest(recv_payload);
    assert(recv_req.session_token == "token456");
    assert(recv_req.game_id == 42);

    std::cout << " passed" << std::endl;
}

void test_list_games_format()
{
    std::cout << "Testing list games format...";

    ListGamesRequest req("token789");
    std::string payload = MessageSerializer::serialize(req);
    std::string message = MessageSerializer::wrapMessage(MessageType::REQ_LIST_GAMES, payload);

    // Verify
    auto [type, recv_payload] = MessageSerializer::unwrapMessage(message);
    assert(type == MessageType::REQ_LIST_GAMES);

    ListGamesRequest recv_req = MessageSerializer::deserializeListGamesRequest(recv_payload);
    assert(recv_req.session_token == "token789");

    std::cout << " passed" << std::endl;
}

void test_disconnect_format()
{
    std::cout << "Testing disconnect format...";

    DisconnectMessage msg("User quit");
    std::string payload = MessageSerializer::serialize(msg);
    std::string message = MessageSerializer::wrapMessage(MessageType::DISCONNECT, payload);

    // Verify
    auto [type, recv_payload] = MessageSerializer::unwrapMessage(message);
    assert(type == MessageType::DISCONNECT);

    DisconnectMessage recv_msg = MessageSerializer::deserializeDisconnect(recv_payload);
    assert(recv_msg.reason == "User quit");

    std::cout << " passed" << std::endl;
}

void test_message_size_limits()
{
    std::cout << "Testing message size limits...";

    // Create a very long player name
    std::string long_name(Protocol::MAX_MESSAGE_SIZE, 'X');
    ConnectRequest req(long_name);
    std::string payload = MessageSerializer::serialize(req);

    // Should exceed max size
    assert(!MessageSerializer::isValidMessageSize(payload));

    // Normal name should be fine
    ConnectRequest normal_req("Alice");
    std::string normal_payload = MessageSerializer::serialize(normal_req);
    assert(MessageSerializer::isValidMessageSize(normal_payload));

    std::cout << " passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running Client Message Tests ===" << std::endl;

    try
    {
        test_connect_request_format();
        test_move_request_format();
        test_create_game_format();
        test_join_game_format();
        test_list_games_format();
        test_disconnect_format();
        test_message_size_limits();

        std::cout << "\n=== All Client message tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}