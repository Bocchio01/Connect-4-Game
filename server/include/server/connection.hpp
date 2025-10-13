#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <functional>
#include <sockpp/tcp_socket.h>

class GameServer;

/**
 * Represents a single client connection
 * Handles reading/writing messages to/from a client
 */
class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using MessageCallback = std::function<void(const std::string &)>;
    using DisconnectCallback = std::function<void()>;

    explicit Connection(sockpp::tcp_socket socket, uint32_t id);
    ~Connection();

    /**
     * Start reading from this connection (blocking in thread)
     */
    void start();

    /**
     * Send a message to the client
     */
    bool send(const std::string &message);

    /**
     * Close the connection
     */
    void close();

    /**
     * Check if connection is open
     */
    bool isOpen() const { return socket_.is_open(); }

    /**
     * Get connection ID
     */
    uint32_t getId() const { return id_; }

    /**
     * Set callback for received messages
     */
    void setMessageCallback(MessageCallback callback)
    {
        message_callback_ = callback;
    }

    /**
     * Set callback for disconnection
     */
    void setDisconnectCallback(DisconnectCallback callback)
    {
        disconnect_callback_ = callback;
    }

private:
    /**
     * Read a length-prefixed message
     * Format: [4 bytes length][message data]
     */
    std::string readMessage();

    /**
     * Write a length-prefixed message
     */
    bool writeMessage(const std::string &message);

    sockpp::tcp_socket socket_;
    uint32_t id_;
    MessageCallback message_callback_;
    DisconnectCallback disconnect_callback_;
    bool running_;
};