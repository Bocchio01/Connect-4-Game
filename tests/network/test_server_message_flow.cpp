#include <iostream>
#include <cassert>

#include "protocol/protocol.hpp"
#include "server/session_manager.hpp"
#include "server/game_session.hpp"

void test_connect_flow()
{
    std::cout << "Testing connection flow...";

    SessionManager session_mgr;

    // Client creates connect request
    ConnectRequest req("TestPlayer");
    std::string payload = MessageSerializer::serialize(req);
    std::string message = MessageSerializer::wrapMessage(MessageType::REQ_CONNECT, payload);

    std::cout << "  Client message: " << message.substr(0, 50) << "...";

    // Server receives and unwraps
    auto [type, recv_payload] = MessageSerializer::unwrapMessage(message);
    assert(type == MessageType::REQ_CONNECT);

    ConnectRequest recv_req = MessageSerializer::deserializeConnectRequest(recv_payload);
    assert(recv_req.player_name == "TestPlayer");

    // Server creates session
    uint32_t conn_id = 1;
    std::string token = session_mgr.createSession(conn_id, recv_req.player_name);

    // Server creates response
    ConnectResponse resp;
    resp.success = true;
    resp.session_token = token;
    resp.message = "Connected";

    std::string resp_payload = MessageSerializer::serialize(resp);
    std::string resp_msg = MessageSerializer::wrapMessage(MessageType::RES_CONNECT, resp_payload);

    // Client receives response
    auto [resp_type, resp_recv_payload] = MessageSerializer::unwrapMessage(resp_msg);
    assert(resp_type == MessageType::RES_CONNECT);

    ConnectResponse recv_resp = MessageSerializer::deserializeConnectResponse(resp_recv_payload);
    assert(recv_resp.success);
    assert(recv_resp.session_token == token);

    std::cout << " passed" << std::endl;
}

void test_move_flow()
{
    std::cout << "Testing move flow...";

    SessionManager session_mgr;
    GameSession game(1, 6, 7, 2, 4);

    // Setup: two players connected
    std::string token1 = session_mgr.createSession(1, "Player1");
    std::string token2 = session_mgr.createSession(2, "Player2");

    uint8_t p1 = game.addPlayer(1);
    uint8_t p2 = game.addPlayer(2);

    session_mgr.assignToGame(token1, 1, p1);
    session_mgr.assignToGame(token2, 1, p2);

    // Player 1 makes a move
    MakeMoveRequest move_req(token1, 3);
    std::string move_payload = MessageSerializer::serialize(move_req);
    std::string move_msg = MessageSerializer::wrapMessage(MessageType::MAKE_MOVE, move_payload);

    // Server receives move
    auto [move_type, move_recv_payload] = MessageSerializer::unwrapMessage(move_msg);
    assert(move_type == MessageType::MAKE_MOVE);

    MakeMoveRequest recv_move = MessageSerializer::deserializeMakeMoveRequest(move_recv_payload);

    // Validate token
    auto conn_id = session_mgr.validateToken(recv_move.session_token);
    assert(conn_id.has_value());
    assert(conn_id.value() == 1);

    // Get session
    auto session = session_mgr.getSessionByConnection(game.getEngine().getState().getCurrentPlayer());
    assert(session.has_value());

    // Apply move
    bool success = game.getEngine().makeMove(recv_move.column, session->player_id);
    assert(success);

    // Verify move was applied
    const auto &board = game.getEngine().getState().getBoard();
    assert(board.get(5, 3) == session->player_id); // Bottom row

    std::cout << " passed" << std::endl;
}

void test_error_handling()
{
    std::cout << "Testing error handling...";

    SessionManager session_mgr;
    GameSession game(1, 6, 7, 2, 4);

    std::string token = session_mgr.createSession(1, "Player1");
    game.addPlayer(1);
    game.addPlayer(2); // Game starts
    session_mgr.assignToGame(token, 1, 1);

    // Try invalid move (out of bounds)
    MakeMoveRequest invalid_move(token, 10); // Column 10 doesn't exist

    auto session = session_mgr.getSessionByConnection(game.getEngine().getState().getCurrentPlayer());
    bool success = game.getEngine().makeMove(invalid_move.column, session->player_id);
    assert(!success);

    // Error should be sent
    ErrorMessage err(Protocol::ErrorCode::INVALID_MOVE, "Column out of range");
    std::string err_payload = MessageSerializer::serialize(err);
    std::string err_msg = MessageSerializer::wrapMessage(MessageType::ERROR, err_payload);

    // Client receives error
    auto [err_type, err_recv_payload] = MessageSerializer::unwrapMessage(err_msg);
    assert(err_type == MessageType::ERROR);

    ErrorMessage recv_err = MessageSerializer::deserializeError(err_recv_payload);
    assert(recv_err.error_code == Protocol::ErrorCode::INVALID_MOVE);

    std::cout << " passed" << std::endl;
}

void test_game_state_update()
{
    std::cout << "Testing game state update...";

    GameSession game(42, 6, 7, 2, 4);

    game.addPlayer(1);
    game.addPlayer(2);

    // Make some moves
    int current_player = game.getEngine().getState().getCurrentPlayer();
    game.getEngine().makeMove(3, current_player);
    game.getEngine().makeMove(3); // Next player's turn

    // Create state update
    GameStateUpdate update;
    update.game_id = game.getId();
    update.rows = game.getEngine().getState().getBoard().getRows();
    update.cols = game.getEngine().getState().getBoard().getCols();
    update.board = game.getEngine().getState().getBoard().getData();
    update.current_player = game.getEngine().getState().getCurrentPlayer();
    update.status = ProtocolGameStatus::IN_PROGRESS;
    update.players = {1, 2};

    // Serialize
    std::string payload = MessageSerializer::serialize(update);
    std::string message = MessageSerializer::wrapMessage(MessageType::GAME_STATE_UPDATE, payload);

    // Client receives
    auto [type, recv_payload] = MessageSerializer::unwrapMessage(message);
    assert(type == MessageType::GAME_STATE_UPDATE);

    GameStateUpdate recv_update = MessageSerializer::deserializeGameStateUpdate(recv_payload);

    assert(recv_update.game_id == 42);
    assert(recv_update.rows == 6);
    assert(recv_update.cols == 7);
    assert(recv_update.board[5][3] == current_player);           // First move
    assert(recv_update.board[4][3] == (current_player % 2) + 1); // Second move
    assert(recv_update.status == ProtocolGameStatus::IN_PROGRESS);

    std::cout << " passed" << std::endl;
}

int main()
{
    std::cout << "\n=== Running Message Flow Tests ===" << std::endl;

    try
    {
        test_connect_flow();
        test_move_flow();
        // test_error_handling();
        test_game_state_update();

        std::cout << "\n=== All message flow tests passed! ===" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
        return 1;
    }
}