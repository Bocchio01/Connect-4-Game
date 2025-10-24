#include <iostream>
#include <cassert>
#include "protocol/protocol.hpp"

void test_connect_request()
{
    std::cout << "Testing ConnectRequest serialization...";

    ConnectRequest req("Alice");
    req.protocol_version = Protocol::VERSION;

    // Serialize
    std::string json = MessageSerializer::serialize(req);
    std::cout << " Serialized: " << json << " ";

    // Deserialize
    ConnectRequest deserialized = MessageSerializer::deserializeConnectRequest(json);

    assert(deserialized.protocol_version == req.protocol_version);
    assert(deserialized.player_name == req.player_name);

    std::cout << "passed" << std::endl;
}

void test_connect_response()
{
    std::cout << "Testing ConnectResponse serialization...";

    ConnectResponse resp;
    resp.success = true;
    resp.session_token = "abc123xyz";
    resp.message = "Welcome to the game!";

    // Serialize
    std::string json = MessageSerializer::serialize(resp);
    std::cout << " Serialized: " << json << " ";

    // Deserialize
    ConnectResponse deserialized = MessageSerializer::deserializeConnectResponse(json);

    assert(deserialized.success == resp.success);
    assert(deserialized.session_token == resp.session_token);
    assert(deserialized.message == resp.message);

    std::cout << "passed" << std::endl;
}

void test_make_move_request()
{
    std::cout << "Testing MakeMoveRequest serialization...";

    MakeMoveRequest req("token123", 3);

    // Serialize
    std::string json = MessageSerializer::serialize(req);
    std::cout << " Serialized: " << json << " ";

    // Deserialize
    MakeMoveRequest deserialized = MessageSerializer::deserializeMakeMoveRequest(json);

    assert(deserialized.session_token == req.session_token);
    assert(deserialized.column == req.column);

    std::cout << "passed" << std::endl;
}

void test_game_state_update()
{
    std::cout << "Testing GameStateUpdate serialization...";

    GameStateUpdate update;
    update.game_id = 42;
    update.rows = 6;
    update.cols = 7;
    update.current_player = 1;
    update.status = ProtocolGameStatus::IN_PROGRESS;
    update.winner = std::nullopt;
    update.players = {1, 2};

    // Create a simple board
    update.board.resize(6);
    for (auto &row : update.board)
    {
        row.resize(7, 0);
    }
    update.board[5][3] = 1; // One piece on the board

    // Serialize
    std::string json = MessageSerializer::serialize(update);
    std::cout << "  Serialized (truncated): " << json.substr(0, 100) << "...";

    // Deserialize
    GameStateUpdate deserialized = MessageSerializer::deserializeGameStateUpdate(json);

    assert(deserialized.game_id == update.game_id);
    assert(deserialized.rows == update.rows);
    assert(deserialized.cols == update.cols);
    assert(deserialized.current_player == update.current_player);
    assert(deserialized.status == update.status);
    assert(!deserialized.winner.has_value());
    assert(deserialized.players.size() == 2);
    assert(deserialized.board[5][3] == 1);

    std::cout << "passed" << std::endl;
}

void test_game_over_message()
{
    std::cout << "Testing GameOverMessage serialization...";

    GameOverMessage msg;
    msg.game_id = 42;
    msg.final_status = ProtocolGameStatus::FINISHED_WIN;
    msg.winner = 1;
    msg.message = "Player 1 wins!";

    // Serialize
    std::string json = MessageSerializer::serialize(msg);
    std::cout << " Serialized: " << json << " ";

    // Deserialize
    GameOverMessage deserialized = MessageSerializer::deserializeGameOver(json);

    assert(deserialized.game_id == msg.game_id);
    assert(deserialized.final_status == msg.final_status);
    assert(deserialized.winner.has_value());
    assert(deserialized.winner.value() == 1);
    assert(deserialized.message == msg.message);

    std::cout << "passed" << std::endl;
}

void test_error_message()
{
    std::cout << "Testing ErrorMessage serialization...";

    ErrorMessage err(Protocol::ErrorCode::INVALID_MOVE, "Column is full");

    // Serialize
    std::string json = MessageSerializer::serialize(err);
    std::cout << " Serialized: " << json << " ";

    // Deserialize
    ErrorMessage deserialized = MessageSerializer::deserializeError(json);

    assert(deserialized.error_code == err.error_code);
    assert(deserialized.error_message == err.error_message);

    std::cout << "passed" << std::endl;
}

void test_message_wrapping()
{
    std::cout << "Testing message wrapping...";

    // Create a move request
    MakeMoveRequest req("token", 5);
    std::string payload = MessageSerializer::serialize(req);

    // Wrap it with type
    std::string wrapped = MessageSerializer::wrapMessage(MessageType::MAKE_MOVE, payload);
    std::cout << "  Wrapped: " << wrapped << std::endl;

    // Unwrap it
    auto [type, unwrapped_payload] = MessageSerializer::unwrapMessage(wrapped);

    assert(type == MessageType::MAKE_MOVE);

    // Deserialize the payload
    MakeMoveRequest deserialized = MessageSerializer::deserializeMakeMoveRequest(unwrapped_payload);
    assert(deserialized.session_token == req.session_token);
    assert(deserialized.column == req.column);

    std::cout << "passed" << std::endl;
}

void test_create_game_request()
{
    std::cout << "Testing CreateGameRequest serialization...";

    CreateGameRequest req;
    req.session_token = "token123";
    req.config = GameConfig(8, 10, 3, 5);

    // Serialize
    std::string json = MessageSerializer::serialize(req);
    std::cout << " Serialized: " << json << " ";

    // Deserialize
    CreateGameRequest deserialized = MessageSerializer::deserializeCreateGameRequest(json);

    assert(deserialized.session_token == req.session_token);
    assert(deserialized.config.rows == 8);
    assert(deserialized.config.cols == 10);
    assert(deserialized.config.num_players == 3);
    assert(deserialized.config.connect_length == 5);

    std::cout << "passed" << std::endl;
}

void test_RES_LIST_GAMES()
{
    std::cout << "Testing ListGamesResponse serialization...";

    ListGamesResponse resp;

    GameInfo game1;
    game1.game_id = 1;
    game1.current_players = 1;
    game1.status = ProtocolGameStatus::NOT_STARTED;
    game1.config = GameConfig(6, 7, 2, 4);

    GameInfo game2;
    game2.game_id = 2;
    game2.current_players = 2;
    game2.status = ProtocolGameStatus::IN_PROGRESS;
    game2.config = GameConfig(8, 8, 2, 5);

    resp.games.push_back(game1);
    resp.games.push_back(game2);

    // Serialize
    std::string json = MessageSerializer::serialize(resp);
    std::cout << "  Serialized (truncated): " << json.substr(0, 150) << "...";

    // Deserialize
    ListGamesResponse deserialized = MessageSerializer::deserializeGameListResponse(json);

    assert(deserialized.games.size() == 2);
    assert(deserialized.games[0].game_id == 1);
    assert(deserialized.games[0].current_players == 1);
    assert(deserialized.games[1].game_id == 2);
    assert(deserialized.games[1].status == ProtocolGameStatus::IN_PROGRESS);

    std::cout << "passed" << std::endl;
}

void test_validation()
{
    std::cout << "Testing validation functions...";

    // Valid JSON
    std::string valid = R"({"key": "value"})";
    assert(MessageSerializer::isValidJson(valid));

    // Invalid JSON
    std::string invalid = R"({"key": invalid})";
    assert(!MessageSerializer::isValidJson(invalid));

    // Size validation
    std::string small = "{}";
    assert(MessageSerializer::isValidMessageSize(small));

    // Create a large message (should fail if > MAX_MESSAGE_SIZE)
    std::string large(Protocol::MAX_MESSAGE_SIZE + 1, 'x');
    assert(!MessageSerializer::isValidMessageSize(large));

    std::cout << "passed" << std::endl;
}

void test_message_type_to_string()
{
    std::cout << "Testing messageTypeToString...";

    assert(messageTypeToString(MessageType::REQ_CONNECT) == "REQ_CONNECT");
    assert(messageTypeToString(MessageType::MAKE_MOVE) == "MAKE_MOVE");
    assert(messageTypeToString(MessageType::ERROR) == "ERROR");

    std::cout << "passed" << std::endl;
}

void test_full_message_flow()
{
    std::cout << "Testing full message flow (client -> server simulation)...";

    // 1. Client creates connect request
    ConnectRequest connectReq("TestPlayer");
    std::string connectPayload = MessageSerializer::serialize(connectReq);
    std::string connectWrapped = MessageSerializer::wrapMessage(
        MessageType::REQ_CONNECT, connectPayload);

    std::cout << "  Client sends: " << connectWrapped << std::endl;

    // 2. Server receives and unwraps
    auto [connectType, connectRecvPayload] = MessageSerializer::unwrapMessage(connectWrapped);
    assert(connectType == MessageType::REQ_CONNECT);

    ConnectRequest serverRecvConnect = MessageSerializer::deserializeConnectRequest(connectRecvPayload);
    assert(serverRecvConnect.player_name == "TestPlayer");

    // 3. Server responds
    ConnectResponse connectResp;
    connectResp.success = true;
    connectResp.session_token = "secure_token_123";
    connectResp.message = "Connected successfully";

    std::string respPayload = MessageSerializer::serialize(connectResp);
    std::string respWrapped = MessageSerializer::wrapMessage(
        MessageType::RES_CONNECT, respPayload);

    std::cout << "  Server responds: " << respWrapped.substr(0, 100) << "...";

    // 4. Client receives response
    auto [respType, respRecvPayload] = MessageSerializer::unwrapMessage(respWrapped);
    assert(respType == MessageType::RES_CONNECT);

    ConnectResponse clientRecvResp = MessageSerializer::deserializeConnectResponse(respRecvPayload);
    assert(clientRecvResp.success);
    assert(clientRecvResp.session_token == "secure_token_123");

    // 5. Client makes a move
    MakeMoveRequest moveReq(clientRecvResp.session_token, 3);
    std::string movePayload = MessageSerializer::serialize(moveReq);
    std::string moveWrapped = MessageSerializer::wrapMessage(
        MessageType::MAKE_MOVE, movePayload);

    std::cout << "  Client moves: " << moveWrapped << std::endl;

    // 6. Server receives move
    auto [moveType, moveRecvPayload] = MessageSerializer::unwrapMessage(moveWrapped);
    assert(moveType == MessageType::MAKE_MOVE);

    MakeMoveRequest serverRecvMove = MessageSerializer::deserializeMakeMoveRequest(moveRecvPayload);
    assert(serverRecvMove.session_token == "secure_token_123");
    assert(serverRecvMove.column == 3);

    std::cout << "passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running Protocol Tests ===" << std::endl;
    std::cout << "Protocol Version: " << Protocol::VERSION << std::endl;
    std::cout << "Default Port: " << Protocol::DEFAULT_PORT << std::endl;
    std::cout << "Max Message Size: " << Protocol::MAX_MESSAGE_SIZE << " bytes\n"
              << std::endl;

    try
    {
        test_connect_request();
        test_connect_response();
        test_make_move_request();
        test_game_state_update();
        test_game_over_message();
        test_error_message();
        test_message_wrapping();
        test_create_game_request();
        test_RES_LIST_GAMES();
        test_validation();
        test_message_type_to_string();
        test_full_message_flow();

        std::cout << "\n=== All Protocol tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}