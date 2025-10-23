#include <iomanip>
#include <iostream>
#include "protocol/protocol.hpp"

void printSeparator()
{
    std::cout << std::string(70, '=') << std::endl;
}

void printHeader(const std::string &title)
{
    printSeparator();
    std::cout << "  " << title << std::endl;
    printSeparator();
}

int main()
{
    std::cout << "\n";
    printHeader("Connect X Protocol Usage Example");

    std::cout << "\nProtocol Information:" << std::endl;
    std::cout << "  Version: " << Protocol::VERSION << std::endl;
    std::cout << "  Default Port: " << Protocol::DEFAULT_PORT << std::endl;
    std::cout << "  Max Message Size: " << Protocol::MAX_MESSAGE_SIZE << " bytes" << std::endl;

    // ========================================================================
    // Scenario 1: Client Connection
    // ========================================================================

    printHeader("Scenario 1: Client Connects to Server");

    std::cout << "\n[CLIENT] Creating connection request..." << std::endl;
    ConnectRequest connectReq("Alice");
    std::string connectPayload = MessageSerializer::serialize(connectReq);
    std::string connectMsg = MessageSerializer::wrapMessage(
        MessageType::CONNECT_REQUEST, connectPayload);

    std::cout << "[CLIENT] Sending to server:" << std::endl;
    std::cout << "  " << connectMsg << std::endl;

    std::cout << "\n[SERVER] Receiving connection request..." << std::endl;
    auto [reqType, reqPayload] = MessageSerializer::unwrapMessage(connectMsg);
    std::cout << "[SERVER] Message type: " << messageTypeToString(reqType) << std::endl;

    ConnectRequest receivedReq = MessageSerializer::deserializeConnectRequest(reqPayload);
    std::cout << "[SERVER] Player name: " << receivedReq.player_name << std::endl;
    std::cout << "[SERVER] Assigning player ID 1, game ID 42..." << std::endl;

    ConnectResponse resp;
    resp.success = true;
    resp.assigned_player_id = 1;
    resp.game_id = 42;
    resp.session_token = "abc123xyz_secure_token";
    resp.message = "Welcome to Connect X!";

    std::string respPayload = MessageSerializer::serialize(resp);
    std::string respMsg = MessageSerializer::wrapMessage(
        MessageType::CONNECT_RESPONSE, respPayload);

    std::cout << "\n[SERVER] Sending response:" << std::endl;
    std::cout << "  " << respMsg << std::endl;

    std::cout << "\n[CLIENT] Receiving response..." << std::endl;
    auto [respType, respPayloadRecv] = MessageSerializer::unwrapMessage(respMsg);
    ConnectResponse receivedResp = MessageSerializer::deserializeConnectResponse(respPayloadRecv);

    std::cout << "[CLIENT] Connection status: "
              << (receivedResp.success ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "[CLIENT] Assigned player ID: " << (int)receivedResp.assigned_player_id << std::endl;
    std::cout << "[CLIENT] Game ID: " << receivedResp.game_id << std::endl;
    std::cout << "[CLIENT] Session token: " << receivedResp.session_token << std::endl;
    std::cout << "[CLIENT] Message: " << receivedResp.message << std::endl;

    // ========================================================================
    // Scenario 2: Making a Move
    // ========================================================================

    printHeader("Scenario 2: Player Makes a Move");

    std::cout << "\n[CLIENT] Creating move request for column 3..." << std::endl;
    MakeMoveRequest moveReq(receivedResp.session_token, 3);
    std::string movePayload = MessageSerializer::serialize(moveReq);
    std::string moveMsg = MessageSerializer::wrapMessage(
        MessageType::MAKE_MOVE, movePayload);

    std::cout << "[CLIENT] Sending move:" << std::endl;
    std::cout << "  " << moveMsg << std::endl;

    std::cout << "\n[SERVER] Receiving move..." << std::endl;
    auto [moveType, movePayloadRecv] = MessageSerializer::unwrapMessage(moveMsg);
    MakeMoveRequest receivedMove = MessageSerializer::deserializeMakeMoveRequest(movePayloadRecv);

    std::cout << "[SERVER] Validating session token..." << std::endl;
    std::cout << "[SERVER] Token valid: YES" << std::endl;
    std::cout << "[SERVER] Column: " << (int)receivedMove.column << std::endl;
    std::cout << "[SERVER] Processing move..." << std::endl;

    MoveResult moveResult(true, "Move successful");
    std::string resultPayload = MessageSerializer::serialize(moveResult);
    std::string resultMsg = MessageSerializer::wrapMessage(
        MessageType::MOVE_RESULT, resultPayload);

    std::cout << "[SERVER] Sending move result:" << std::endl;
    std::cout << "  " << resultMsg << std::endl;

    // ========================================================================
    // Scenario 3: Game State Update
    // ========================================================================

    printHeader("Scenario 3: Broadcasting Game State");

    std::cout << "\n[SERVER] Creating game state update..." << std::endl;

    GameStateUpdate stateUpdate;
    stateUpdate.game_id = 42;
    stateUpdate.rows = 6;
    stateUpdate.cols = 7;
    stateUpdate.current_player = 2; // Next player's turn
    stateUpdate.status = ProtocolGameStatus::IN_PROGRESS;
    stateUpdate.winner = std::nullopt;
    stateUpdate.players = {1, 2};

    // Create board with one piece
    stateUpdate.board.resize(6);
    for (auto &row : stateUpdate.board)
    {
        row.resize(7, 0);
    }
    stateUpdate.board[5][3] = 1; // Player 1's piece at bottom of column 3

    std::string statePayload = MessageSerializer::serialize(stateUpdate);
    std::string stateMsg = MessageSerializer::wrapMessage(
        MessageType::GAME_STATE_UPDATE, statePayload);

    std::cout << "[SERVER] Broadcasting to all clients..." << std::endl;
    std::cout << "  Game ID: " << stateUpdate.game_id << std::endl;
    std::cout << "  Current player: " << (int)stateUpdate.current_player << std::endl;
    std::cout << "  Status: IN_PROGRESS" << std::endl;
    std::cout << "  Board state: 1 piece placed" << std::endl;

    std::cout << "\n[CLIENT] Receiving state update..." << std::endl;
    auto [stateType, statePayloadRecv] = MessageSerializer::unwrapMessage(stateMsg);
    GameStateUpdate receivedState = MessageSerializer::deserializeGameStateUpdate(statePayloadRecv);

    std::cout << "[CLIENT] Game state updated" << std::endl;
    std::cout << "[CLIENT] Current turn: Player " << (int)receivedState.current_player << std::endl;
    std::cout << "[CLIENT] Board size: " << (int)receivedState.rows
              << "x" << (int)receivedState.cols << std::endl;

    // ========================================================================
    // Scenario 4: Game Over
    // ========================================================================

    printHeader("Scenario 4: Game Ends");

    std::cout << "\n[SERVER] Player 1 wins! Creating game over message..." << std::endl;

    GameOverMessage gameOver;
    gameOver.game_id = 42;
    gameOver.final_status = ProtocolGameStatus::FINISHED_WIN;
    gameOver.winner = 1;
    gameOver.message = "Player 1 (Alice) wins with 4 in a row!";

    std::string gameOverPayload = MessageSerializer::serialize(gameOver);
    std::string gameOverMsg = MessageSerializer::wrapMessage(
        MessageType::GAME_OVER, gameOverPayload);

    std::cout << "[SERVER] Broadcasting game over:" << std::endl;
    std::cout << "  " << gameOverMsg << std::endl;

    std::cout << "\n[CLIENT] Receiving game over message..." << std::endl;
    auto [gameOverType, gameOverPayloadRecv] = MessageSerializer::unwrapMessage(gameOverMsg);
    GameOverMessage receivedGameOver = MessageSerializer::deserializeGameOver(gameOverPayloadRecv);

    std::cout << "[CLIENT] Game finished!" << std::endl;
    std::cout << "[CLIENT] Winner: Player " << (int)receivedGameOver.winner.value() << std::endl;
    std::cout << "[CLIENT] Message: " << receivedGameOver.message << std::endl;

    // ========================================================================
    // Scenario 5: Error Handling
    // ========================================================================

    printHeader("Scenario 5: Error Handling");

    std::cout << "\n[CLIENT] Attempting invalid move (column already full)..." << std::endl;

    MakeMoveRequest invalidMove(receivedResp.session_token, 3);
    std::string invalidPayload = MessageSerializer::serialize(invalidMove);
    std::string invalidMsg = MessageSerializer::wrapMessage(
        MessageType::MAKE_MOVE, invalidPayload);

    std::cout << "[CLIENT] Sending move..." << std::endl;

    std::cout << "\n[SERVER] Move validation failed!" << std::endl;

    ErrorMessage error(Protocol::ErrorCode::COLUMN_FULL, "Column 3 is full");
    std::string errorPayload = MessageSerializer::serialize(error);
    std::string errorMsg = MessageSerializer::wrapMessage(
        MessageType::ERROR, errorPayload);

    std::cout << "[SERVER] Sending error:" << std::endl;
    std::cout << "  " << errorMsg << std::endl;

    std::cout << "\n[CLIENT] Receiving error..." << std::endl;
    auto [errorType, errorPayloadRecv] = MessageSerializer::unwrapMessage(errorMsg);
    ErrorMessage receivedError = MessageSerializer::deserializeError(errorPayloadRecv);

    std::cout << "[CLIENT] Error code: " << receivedError.error_code << std::endl;
    std::cout << "[CLIENT] Error message: " << receivedError.error_message << std::endl;

    // ========================================================================
    // Summary
    // ========================================================================

    printHeader("Summary");

    std::cout << "\nThe protocol supports:" << std::endl;
    std::cout << "  Connection management (CONNECT_REQUEST, CONNECT_RESPONSE)" << std::endl;
    std::cout << "  Game actions (MAKE_MOVE, MOVE_RESULT)" << std::endl;
    std::cout << "  State synchronization (GAME_STATE_UPDATE)" << std::endl;
    std::cout << "  Game lifecycle (GAME_OVER)" << std::endl;
    std::cout << "  Error handling (ERROR)" << std::endl;
    std::cout << "  Lobby features (CREATE_GAME, JOIN_GAME, LIST_GAMES)" << std::endl;
    std::cout << "  Session security (tokens)" << std::endl;
    std::cout << "\nAll messages are JSON-encoded for easy debugging!" << std::endl;

    printSeparator();
    std::cout << std::endl;

    return 0;
}