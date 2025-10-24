#pragma once

#include <memory>
#include <functional>

#include "core/move.hpp"
#include "core/game/state.hpp"
#include "core/game/rules.hpp"

/**
 * Core game engine - manages game flow and enforces rules
 */
class GameEngine
{
public:
    using StateChangeCallback = std::function<void(const GameState &)>;

    /**
     * Constructor
     * @param rows Board rows
     * @param cols Board columns
     * @param num_players Number of players
     * @param connect_length Number of pieces to connect (default 4)
     */
    GameEngine(uint8_t rows = 6,
               uint8_t cols = 7,
               uint8_t num_players = 2,
               uint8_t connect_length = 4);

    /**
     * Start a new game
     */
    void startGame();

    /**
     * Make a move for the current player
     * @param column Column to drop piece in
     * @return true if move was successful
     */
    bool makeMove(uint8_t column);

    /**
     * Make a move for a specific player (with validation)
     * @param column Column to drop piece in
     * @param player_id Player making the move
     * @return true if move was successful
     */
    bool makeMove(uint8_t column, uint8_t player_id);

    /**
     * Reset the game to initial state
     */
    void reset();

    /**
     * Register callback for state changes
     */
    void setStateChangeCallback(StateChangeCallback callback)
    {
        state_change_callback_ = callback;
    }

    /**
     * Skip one player's turn
     */
    void skipPlayerTurn()
    {
        advancePlayer();
        notifyStateChange();
    }

    // State accessors
    const GameState &getState() const { return state_; }
    const GameRules &getRules() const { return rules_; }

    bool isGameOver() const;
    std::optional<uint8_t> getWinner() const { return state_.getWinner(); }

private:
    GameState state_;
    GameRules rules_;
    StateChangeCallback state_change_callback_;

    void advancePlayer();
    void checkGameEnd(uint8_t last_player);
    void notifyStateChange();
};